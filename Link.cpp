#include <bitset>
#include <cmath>
#include "Link.h"

int num_of_processes;

/// To handle the size of the congestion
vector<int> state;
vector<int> congestion_window_size;
vector<int> ssthresh;
vector<int> duplicate_ack_count;
vector<float> congestion_avoidance_augment;
vector<mutex> mtx_state(MAX_NUMBER_OF_PROCESSES);
vector<mutex> mtx_congestion_window_size(MAX_NUMBER_OF_PROCESSES);
vector<mutex> mtx_ssthresh(MAX_NUMBER_OF_PROCESSES);
vector<mutex> mtx_duplicate_ack_count(MAX_NUMBER_OF_PROCESSES);

/// To handle times
vector<mutex> mtx_timeouts(MAX_NUMBER_OF_PROCESSES);
vector<unsigned long> timeouts;
vector<double> last_srtt;
vector<double> last_sdev;

// to handle concurrency on the incoming_messages queue between the manager of the link and the receiver
bool incoming_messages_locked = false;
mutex mtx_incoming_messages, mtx_acks;
condition_variable cv_incoming_messages;
queue<pp2p_message> incoming_messages;

vector<mutex> mtx_messages_to_send_by_process(MAX_NUMBER_OF_PROCESSES);
vector<queue<pp2p_message>> messages_to_send_by_process;

mutex mtx_outgoing_messages;
queue<pair<int,pp2p_message>> outgoing_messages;

vector<unordered_set<long long int>> acks;  //acks
vector<unordered_set<long long int>> pl_delivered;  //sequence numbers of the delivered messages at perfect link level
                                                    //ordered by the process that sent them


Link::Link(int sockfd, int process_number, unordered_map<int, pair<string, int>> *socket_by_process_id) {
    // Constructor
    this->sockfd = sockfd;
    this->process_number = process_number;
    this->socket_by_process_id = socket_by_process_id;
    this->last_seq_number.resize(this->socket_by_process_id->size() + 1, 0LL);  // Initialize all sequence numbers to zero.

    num_of_processes = socket_by_process_id->size();

    state.resize(num_of_processes, SLOW_START);
    congestion_window_size.resize(num_of_processes, 1);
    ssthresh.resize(num_of_processes, 512);
    duplicate_ack_count.resize(num_of_processes, 0);
    congestion_avoidance_augment.resize(num_of_processes, 0);
    timeouts.resize(num_of_processes, DEFAULT_TIMEOUT);
    last_srtt.resize(num_of_processes, DEFAULT_TIMEOUT);
    last_sdev.resize(num_of_processes, 0);

    messages_to_send_by_process.resize(num_of_processes);
    for (int i = 0; i < num_of_processes; i++) {
        queue<pp2p_message> q;
        messages_to_send_by_process[i] = q;
    }
}


/**
 * This method spawns the threads that will be used to send and receive messages on a port
 */
void Link::init() {
    thread t_rec(run_receiver, this);
    thread t_send(run_sender, this->socket_by_process_id, this->sockfd);
    t_rec.detach();
    t_send.detach();
}


int Link::get_sockfd(){
    return this->sockfd;
}


int Link::get_process_number() {
    return this->process_number;
}


/**
 * Format of messages:
 * - 1 bit for ack: 0 normal message, 1 ack.
 *
 * CAREFUL HERE: the message is not ready to run, first the sequence number must be added.
 *
 * This method only puts the message in the queue of outgoing messages, so that the sender can send it.
 * @param d_process_number destination address.
 * @param msg the message to send
 */
void Link::send_to(int d_process_number, pp2p_message& msg) {
    long long seq_number = this->last_seq_number[d_process_number] ++;
    msg.seq_number = seq_number;
    mtx_messages_to_send_by_process[d_process_number - 1].lock();
    messages_to_send_by_process[d_process_number - 1].push(msg);
    mtx_messages_to_send_by_process[d_process_number - 1].unlock();
}



/**
 * Method used to send an acks at link level, in other words when a message is received an ack is sent to the broadcaster
 */
void Link::send_ack(pp2p_message &msg) {
    int dest_process = msg.proc_number;

    pp2p_message ack_message(true, msg.seq_number, process_number, msg.timestamp, msg.payload); //the ack should have the same timestamp of the message we are acking

    struct sockaddr_in d_addr;

    memset(&d_addr, 0, sizeof(d_addr));
    d_addr.sin_family = AF_INET;
    d_addr.sin_port = (*socket_by_process_id)[dest_process].second;
    string ip_address = (*socket_by_process_id)[dest_process].first;
    // wrong line here
    inet_pton(AF_INET, ip_address.c_str(), &(d_addr.sin_addr));
    string message_to_send = to_string(ack_message);
    sendto(sockfd, message_to_send.c_str(), strlen(message_to_send.c_str()),
           MSG_CONFIRM, (const struct sockaddr *) &d_addr,
           sizeof(d_addr));

}

/**
 * This methods get the incoming messages and then handles both the sending of the ack and the delivery at perfect link level.
 *
 * It avoids duplicate messages.
 * @return the next new message.
 */
pp2p_message Link::get_next_message(){
    while(true){
        unique_lock<mutex> lck(mtx_incoming_messages);
        cv_incoming_messages.wait(lck, [&] { return !incoming_messages.empty(); });
        incoming_messages_locked = true;
        pp2p_message msg = incoming_messages.front();
        incoming_messages.pop();
        incoming_messages_locked = false;
        cv_incoming_messages.notify_one();

        if (msg.ack) {
            mtx_acks.lock();
            bool duplicate = !((acks[msg.proc_number].insert(msg.seq_number)).second);
            mtx_acks.unlock();

            if (duplicate)
                on_duplicate_ack(msg.proc_number);
            else {
                on_new_ack(msg.proc_number);
                update_times(msg.proc_number, msg.timestamp);
            }

        } else {
            this->send_ack(msg);
            // Check if it has not been delivered already
            if (pl_delivered[msg.proc_number].insert(msg.seq_number).second) {
#ifdef DEBUG
                cout << "(" << process_number << ") message delivered: (" << msg.payload.first_sender << "," << msg.payload.seq_number << ")" << endl;
#endif
                return msg;
            }

        }
        mtx_pp2p_get_msg.lock();
        if (stop_pp2p_get_msg){
            mtx_pp2p_get_msg.unlock();
            break;
        }
        mtx_pp2p_get_msg.unlock();
    }
    // This happens only when the process is killed, no harm can be done!
    pp2p_message fake = create_fake_pp2p();
    return fake;
}


/**
 * Method run by the thread sender, it dequeues outgoing messages and then sends
 *
 * @param socket_by_process_id  data structure that maps every process (number) to its (ip, port) pair
 * @param sockfd the socket fd of the link's owner process
 */
void run_sender(unordered_map<int, pair<string, int>> *socket_by_process_id, int sockfd) {
    struct sockaddr_in d_addr;
    while (true) {

        mtx_outgoing_messages.lock();
        if (!outgoing_messages.empty()) {
            pair<int, pp2p_message> dest_and_msg = outgoing_messages.front();
            outgoing_messages.pop();
            mtx_outgoing_messages.unlock();

            mtx_acks.lock();
            if (acks[dest_and_msg.first].find(dest_and_msg.second.seq_number) == acks[dest_and_msg.first].end()) {
                // Send only if the ack hasn't been received
                mtx_acks.unlock();

                if (!dest_and_msg.second.ack) {
                    if (dest_and_msg.second.timestamp != 0UL) {
                        // This means that the message has already been previously sent
                        if (time_milli() < dest_and_msg.second.timestamp + get_timeout(dest_and_msg.first)) {
                            // If the timeout is not expired yet, push the message in the queue again
                            mtx_outgoing_messages.lock();
                            outgoing_messages.push(dest_and_msg);
                            mtx_outgoing_messages.unlock();
                            continue;
                        } else
                            // Otherwise signal that a timeout has expired and then proceed like for a normal message to send it
                            on_timeout(dest_and_msg.first);
                    }
                    // Add timestamp
                    dest_and_msg.second.timestamp = time_milli();

                    mtx_outgoing_messages.lock();
                    outgoing_messages.push(dest_and_msg);
                    mtx_outgoing_messages.unlock();
                }

                string msg_s = to_string(dest_and_msg.second);
                const char *msg_c = msg_s.c_str();

                memset(&d_addr, 0, sizeof(d_addr));
                d_addr.sin_family = AF_INET;
                d_addr.sin_port = (*socket_by_process_id)[dest_and_msg.first].second;
                string ip_address = (*socket_by_process_id)[dest_and_msg.first].first;
                inet_pton(AF_INET, ip_address.c_str(), &(d_addr.sin_addr));

                sendto(sockfd, msg_c, strlen(msg_c),
                       MSG_CONFIRM, (const struct sockaddr *) &d_addr,
                       sizeof(d_addr));
#ifdef DEBUG
                cout << "Sent message: (" << dest_and_msg.second.payload.first_sender << "," << dest_and_msg.second.payload.seq_number << ") to " << dest_and_msg.first << endl;
#endif

            } else {
                mtx_acks.unlock();
            }

        } else
            mtx_outgoing_messages.unlock();

        mtx_pp2p_sender.lock();
        if (stop_pp2p_sender) {
            mtx_pp2p_sender.unlock();
            break;
        }
        mtx_pp2p_sender.unlock();
    }
}


/**
 * This is the method run by the thread receiver, it listens on a certain port until it gets a message and then it
 * puts the message the incoming_messages queue
 *
 * @param link
 */
void run_receiver(Link *link) {
    while (true) {
        unsigned int len;
        char buf[1024];
        struct sockaddr_in sender_addr;
        int n = recvfrom(link->get_sockfd(), (char *)buf, MAXLINE, MSG_WAITALL, ( struct sockaddr *) &sender_addr,
                         &len);

        if(n == 0)
            break;

        buf[n] = '\0';

        pp2p_message msg = parse_message(string(buf));

#ifdef DEBUG
        cout << "\nReceived message: (" << msg.payload.first_sender << "," << msg.payload.seq_number << ")" << endl;
#endif

        // Put the message in the queue.
        unique_lock<mutex> lck(mtx_incoming_messages);
        cv_incoming_messages.wait(lck, [&] { return !incoming_messages_locked; });
        incoming_messages_locked = true;
        incoming_messages.push(msg);
        incoming_messages_locked = false;
        cv_incoming_messages.notify_one();

        // stop in case da_proc received a sigterm!
        mtx_pp2p_receiver.lock();
        if (stop_pp2p_receiver){
            mtx_pp2p_receiver.unlock();
            break;
        }
        mtx_pp2p_receiver.unlock();
    }
}


void on_new_ack(int proc_num) {
    int messages_to_send = 0;
    if(get_state(proc_num) == SLOW_START) {
        messages_to_send = increase_congestion_window_size(proc_num, SLOW_START);
        if (get_congestion_window_size(proc_num) >= get_ssthresh(proc_num))
            set_state(proc_num, CONGESTION_AVOIDANCE);
    } else if (get_state(proc_num) == CONGESTION_AVOIDANCE) {
        messages_to_send = increase_congestion_window_size(proc_num, CONGESTION_AVOIDANCE);
    } else if (get_state(proc_num) == FAST_RECOVERY) {
        messages_to_send = set_congestion_window_size(proc_num, get_ssthresh(proc_num));
        set_state(proc_num, CONGESTION_AVOIDANCE);
    }
    reset_duplicate_ack_count(proc_num);
    enqueue_new_messages(proc_num, messages_to_send);
}


void on_duplicate_ack(int proc_num) {

    //log("duplicate", proc_num);

    int messages_to_send = 0;
    int s = get_state(proc_num);
    if (s == SLOW_START || s == CONGESTION_AVOIDANCE) {
        increase_duplicate_ack_count(proc_num);
        if (get_duplicate_ack_count(proc_num) == 3) {
            set_state(proc_num, FAST_RECOVERY);
            set_ssthresh(proc_num, int(get_congestion_window_size(proc_num) / 2) + 1);
            messages_to_send = set_congestion_window_size(proc_num, get_ssthresh(proc_num) + 3);
        }
    } else if (s == FAST_RECOVERY)
        messages_to_send = increase_congestion_window_size(proc_num, SLOW_START);
    enqueue_new_messages(proc_num, messages_to_send);
}


void on_timeout(int proc_num) {
    set_ssthresh(proc_num, int(get_congestion_window_size(proc_num) / 2) + 1);
    set_congestion_window_size(proc_num, 1);
    reset_duplicate_ack_count(proc_num);
    set_state(proc_num, SLOW_START);
    set_timeout(proc_num, DEFAULT_TIMEOUT);
}


void enqueue_new_messages(int proc_num, int number_of_messages) {
    int count = number_of_messages;

    mtx_messages_to_send_by_process[proc_num - 1].lock();
    while (true) {

        if(messages_to_send_by_process[proc_num - 1].empty()) {
            mtx_messages_to_send_by_process[proc_num - 1].unlock();
            break;
        }

        pp2p_message msg = messages_to_send_by_process[proc_num - 1].front();
        messages_to_send_by_process[proc_num - 1].pop();
        mtx_messages_to_send_by_process[proc_num - 1].unlock();

        mtx_outgoing_messages.lock();
        outgoing_messages.push({proc_num, msg});
        mtx_outgoing_messages.unlock();

        count--;
        if (count == 0)
            break;

        mtx_messages_to_send_by_process[proc_num - 1].lock();
    }
}


int get_state(int proc_num) {
    mtx_state[proc_num - 1].lock();
    int s = state[proc_num - 1];
    mtx_state[proc_num - 1].unlock();
    return s;
}


int get_congestion_window_size(int proc_num) {
    mtx_congestion_window_size[proc_num - 1].lock();
    int size = congestion_window_size[proc_num - 1];
    mtx_congestion_window_size[proc_num - 1].unlock();

    /*
    for (int i = 0; i < 3; i++){
        mtx_congestion_window_size[i].lock();
        log(to_string(congestion_window_size[i]), i + 1);
        mtx_congestion_window_size[i].unlock();
    }
     */

    return size;
}


int get_ssthresh(int proc_num) {
    mtx_ssthresh[proc_num - 1].lock();
    int size = ssthresh[proc_num - 1];
    mtx_ssthresh[proc_num - 1].unlock();
    return size;
}


int get_duplicate_ack_count(int proc_num) {
    mtx_duplicate_ack_count[proc_num - 1].lock();
    int count = duplicate_ack_count[proc_num - 1];
    mtx_duplicate_ack_count[proc_num - 1].unlock();
    return count;
}


unsigned long get_timeout(int proc_num) {
    mtx_timeouts[proc_num - 1].lock();
    unsigned long tout = timeouts[proc_num - 1];
    mtx_timeouts[proc_num - 1].unlock();
    return tout;
}


void set_state(int proc_num, int new_value) {
    mtx_state[proc_num - 1].lock();
    state[proc_num - 1] = new_value;
    mtx_state[proc_num - 1].unlock();
}


/**
 * Setter for congestion window's size
 * @param proc_num the process of which the window should be set to the desired value
 * @param new_value the value to which set the window size
 * @return the number of messages to send to the process <proc_num>
 */
int set_congestion_window_size(int proc_num, int new_value) {
    mtx_congestion_window_size[proc_num - 1].lock();
    int old_value = congestion_window_size[proc_num - 1];
    congestion_window_size[proc_num - 1] = new_value;
    mtx_congestion_window_size[proc_num - 1].unlock();
    return max(0, new_value - old_value);
}


/**
 * Increases congestion window's size according to the parameter state
 * @param proc_num the process of which the window should be increased
 * @param st the state of the connection with the process <proc_num>
 * @return the number of messages to send to the process <proc_num>
 */
int increase_congestion_window_size(int proc_num, int st) {
    bool size_incremented = false;
    mtx_congestion_window_size[proc_num - 1].lock();
    if (st == SLOW_START) {
        congestion_window_size[proc_num - 1]++;
        size_incremented = true;
    } else if (st == CONGESTION_AVOIDANCE) {
        congestion_avoidance_augment[proc_num - 1] += 1.0 / float(congestion_window_size[proc_num - 1]);
    }
    mtx_congestion_window_size[proc_num - 1].unlock();

    if (congestion_avoidance_augment[proc_num - 1] >= 1.0)
        size_incremented = true;

    return size_incremented ? 1 : 0;
}


void set_ssthresh(int proc_num, int new_value) {
    mtx_ssthresh[proc_num - 1].lock();
    ssthresh[proc_num - 1] = new_value;
    mtx_ssthresh[proc_num - 1].unlock();
}


void reset_duplicate_ack_count(int proc_num) {
    mtx_duplicate_ack_count[proc_num - 1].lock();
    duplicate_ack_count[proc_num - 1] = 0;
    mtx_duplicate_ack_count[proc_num - 1].unlock();
}


void increase_duplicate_ack_count(int proc_num) {
    mtx_duplicate_ack_count[proc_num - 1].lock();
    duplicate_ack_count[proc_num - 1]++;
    mtx_duplicate_ack_count[proc_num - 1].unlock();
}


void set_timeout(int proc_num, unsigned long new_value) {
    mtx_timeouts[proc_num - 1].lock();
    timeouts[proc_num - 1] = new_value;
    mtx_timeouts[proc_num - 1].unlock();

    /*
    for (int i = 0; i < 3; i++){
        mtx_timeouts[i].lock();
        log(to_string(timeouts[i]), i + 1);
        mtx_timeouts[i].unlock();
    }
     */

}


unsigned long time_milli() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
    ).count();
}


void update_times(int proc_number, unsigned long sending_time) {
    auto curr_rtt = double(time_milli() - sending_time);
    double curr_srtt = (1 - SMOOTHNESS) * last_srtt[proc_number - 1] + SMOOTHNESS * curr_rtt;
    double dev = (curr_rtt > last_srtt[proc_number - 1])
                        ? curr_rtt - last_srtt[proc_number - 1]
                        : last_srtt[proc_number - 1] - curr_rtt;
    last_srtt[proc_number - 1] = curr_srtt;
    double curr_sdev = 0.75 * last_sdev[proc_number - 1] + 0.25 * dev;
    last_sdev[proc_number - 1] = curr_sdev;
    unsigned long new_timeout = ceil(curr_srtt + 4 * curr_sdev);
    if(new_timeout > 100)
        set_timeout(proc_number, new_timeout);
}


