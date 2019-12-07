#include <bitset>
#include "Link.h"

//to handle concurrency on the incoming_messages queue between the manager of the link and the receiver
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

    for (unsigned i = 0; i < socket_by_process_id->size(); i++) {
        queue<pp2p_message> q;
        messages_to_send_by_process.push_back(q);
        state.push_back(SLOW_START);
        congestion_window_size.push_back(1);
        ssthresh.push_back(512);
        duplicate_ack_count.push_back(0);
        congestion_avoidance_augment.push_back(0);
    }
}


/**
 * This method spawns the threads that will be used to send and receive messages on a port
 */
void Link::init() {
    thread t_rec(run_receiver, this);
    thread t_send(run_sender, this->socket_by_process_id, this->sockfd);
    //thread t_mod(run_moderator);
    t_rec.detach();
    t_send.detach();
    //t_mod.detach();
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
    // At the moment, we keep a queue of messages to send and not acked yet.
    // Sending a pp2p_message consist easily in putting a new pp2p_message in the queue, and
    // wait until the run_sender method sends it.

    // Before doing it, we must choose a proper sequence number.
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
    // Create the ack string (1-source process-sequence number of pp2p_message to ack)
    int source_process = process_number;
    int dest_process = msg.proc_number;

    pp2p_message ack_message(true, msg.seq_number, source_process, msg.payload);

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
#ifdef DEBUG
        cout << "get next message" << endl;
#endif
        if (msg.ack) {
            mtx_acks.lock();
            bool duplicate = !((acks[msg.proc_number].insert(msg.seq_number)).second);
            mtx_acks.unlock();

            if (duplicate)
                on_duplicate_ack(msg.proc_number);
            else
                on_new_ack(msg.proc_number);

        } else {
            this->send_ack(msg);
            // Check if it has not been delivered already
            if (pl_delivered[msg.proc_number].insert(msg.seq_number).second)
                return msg;
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


void Link::on_new_ack(int proc_num) {
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
    enqueue_messages(proc_num, messages_to_send);
}


void Link::on_duplicate_ack(int proc_num) {
    int messages_to_send = 0;
    int s = get_state(proc_num);
    if (s == SLOW_START || s == CONGESTION_AVOIDANCE) {
        increase_duplicate_ack_count(proc_num);
        if (get_duplicate_ack_count(proc_num) == 3) {
            set_state(proc_num, FAST_RECOVERY);
            set_ssthresh(proc_num, get_congestion_window_size(proc_num) / 2);
            messages_to_send = set_congestion_window_size(proc_num, get_ssthresh(proc_num) + 3);
        } else if (get_state(proc_num) == FAST_RECOVERY) {
            messages_to_send = increase_congestion_window_size(proc_num, SLOW_START);
        }
    }
    enqueue_messages(proc_num, messages_to_send);
}


void Link::on_timeout(int proc_num) {
    set_ssthresh(proc_num, get_congestion_window_size(proc_num) / 2);
    set_congestion_window_size(proc_num, 1);
    reset_duplicate_ack_count(proc_num);
    set_state(proc_num, SLOW_START);
}


void Link::enqueue_messages(int proc_num, int number_of_messages) {
    vector<pp2p_message> messages_to_send;
    int count = number_of_messages;

    mtx_messages_to_send_by_process[proc_num - 1].lock();
    while (!messages_to_send_by_process[proc_num].empty() || count != 0) {
        pp2p_message msg = messages_to_send_by_process[proc_num - 1].front();
        messages_to_send.push_back(msg);
        messages_to_send_by_process[proc_num - 1].pop();
        count--;
    }
    mtx_messages_to_send_by_process[proc_num].unlock();

    mtx_outgoing_messages.lock();
    for (int i=0; i<number_of_messages; i++) {
        outgoing_messages.push({proc_num, messages_to_send[i]});
    }
    mtx_outgoing_messages.unlock();
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
        //int curr_dest;
        //vector<pair<int,pp2p_message>> window(0);

        mtx_outgoing_messages.lock();
        if (!outgoing_messages.empty()) {
            pair<int, pp2p_message> dest_and_msg = outgoing_messages.front();
            outgoing_messages.pop();
            /*
            curr_dest = dest_and_msg.first;

            while(!outgoing_messages.empty() || curr_dest != dest_and_msg.first) {
                // until I get an ack or a message to send to a process different
                // from the curr_dest I keep putting messages into my window
                window.push_back(dest_and_msg);
                outgoing_messages.pop();
                dest_and_msg = outgoing_messages.front();
            }
             */

            mtx_outgoing_messages.unlock();

            /*
            for (int i = 0; i < window.size(); i++) {
                string msg_s = to_string(window[i].second);
                const char *msg_c = msg_s.c_str();

                memset(&d_addr, 0, sizeof(d_addr));
                d_addr.sin_family = AF_INET;
                d_addr.sin_port = (*socket_by_process_id)[dest_and_msg.first].second;
                string ip_address = (*socket_by_process_id)[dest_and_msg.first].first;
                // wrong line here
                inet_pton(AF_INET, ip_address.c_str(), &(d_addr.sin_addr));

                sendto(sockfd, msg_c, strlen(msg_c),
                       MSG_CONFIRM, (const struct sockaddr *) &d_addr,
                       sizeof(d_addr));
            }

            update_time(curr_dest);

             */

            mtx_acks.lock();
            if (acks[dest_and_msg.first].find(dest_and_msg.second.seq_number) == acks[dest_and_msg.first].end()) {
                // Send only if the ack hasn't been received
                mtx_acks.unlock();
                string msg_s = to_string(dest_and_msg.second);
                //TODO aggiungere timestamp
                const char *msg_c = msg_s.c_str();

                memset(&d_addr, 0, sizeof(d_addr));
                d_addr.sin_family = AF_INET;
                d_addr.sin_port = (*socket_by_process_id)[dest_and_msg.first].second;
                string ip_address = (*socket_by_process_id)[dest_and_msg.first].first;
                // wrong line here
                inet_pton(AF_INET, ip_address.c_str(), &(d_addr.sin_addr));

                sendto(sockfd, msg_c, strlen(msg_c),
                       MSG_CONFIRM, (const struct sockaddr *) &d_addr,
                       sizeof(d_addr));
#ifdef DEBUG
                cout << "Sent " << msg_c << " to " << dest_and_msg.first << endl;
#endif
                if (!dest_and_msg.second.ack) {
                    //TODO aggiungere con priorità
                    mtx_messages_to_send_by_process[dest_and_msg.first - 1].lock();
                    messages_to_send_by_process[dest_and_msg.first - 1].push(dest_and_msg.second);
                    mtx_messages_to_send_by_process[dest_and_msg.first - 1].unlock();
                }
            } else {
                mtx_acks.unlock();
            }

        } else
            mtx_outgoing_messages.unlock();

        // wait a bit before sending the new message.
        // usleep(10);
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
        buf[n] = '\0';

        pp2p_message msg = parse_message(string(buf));

#ifdef DEBUG
        cout << "\nNew message " << msg.proc_number << " " << msg.seq_number << endl;
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

/*
void run_moderator() {
    int pn = 0;

    while (true) {
        mtx_messages_to_send_by_process[pn].lock();
        if (!messages_to_send_by_process[pn].empty()) {
            pp2p_message msg = messages_to_send_by_process[pn].front();

            mtx_outgoing_messages.lock();
            outgoing_messages.push({pn + 1, msg});
            mtx_outgoing_messages.unlock();

            messages_to_send_by_process[pn].pop();
        }
        mtx_messages_to_send_by_process[pn].unlock();

        pn++;
        if (pn == num_of_processes)
            pn = 0;
    }
}
 */

