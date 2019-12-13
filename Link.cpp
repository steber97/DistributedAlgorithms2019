#include "Link.h"

// To handle concurrency on the message queues between the receiver and the sender
bool queue_incoming_messages_locked = false;
mutex mtx_incoming_messages, mtx_outgoing_messages, mtx_acks;
condition_variable cv_incoming_messages;

// These two data structure contain the incoming and the outgoing messages
queue<pp2p_message> incoming_messages;
queue<pp2p_message> outgoing_messages;

vector<unordered_set<long long int>> acks;
vector<unordered_set<long long int>> pl_delivered;  // sequence numbers of the delivered messages at perfect link level
                                                    // ordered by the process that sent them


Link::Link(int sockfd, int process_number, unordered_map<int, pair<string, int>> *socket_by_process_id, const int number_of_processes) {
    // Constructor
    this->sockfd = sockfd;
    this->process_number = process_number;
    this->socket_by_process_id = socket_by_process_id;
    // the size of last_seq_number is longer by 1, as processes start from 1
    this->last_seq_number.resize(this->socket_by_process_id->size() + 1, 0LL);  // Initialize all sequence numbers to zero.
    this->number_of_processes = number_of_processes;
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
    // At the moment, we keep a queue of messages to send and not acked yet.
    // Sending a pp2p_message consist easily in putting a new pp2p_message in the queue, and
    // wait until the run_sender method sends it.

    // Before doing it, we must choose a proper sequence number.
    long long seq_number = this->last_seq_number[d_process_number] ++;
    msg.seq_number = seq_number;
    mtx_outgoing_messages.lock();
    outgoing_messages.push(msg);
    mtx_outgoing_messages.unlock();
}


/**
 * Method used to send an acks at link level, in other words when a message is received an ack is sent to the broadcaster
 */
void Link::send_ack(pp2p_message &msg) {
    int source_process = process_number;
    int dest_process = msg.proc_number;

    // here the destination process number is just useless, we already know it (it is the sender of the message).
    pp2p_message ack_message(true, msg.seq_number, source_process, -1, msg.payload);

    struct sockaddr_in d_addr;

    memset(&d_addr, 0, sizeof(d_addr));
    d_addr.sin_family = AF_INET;
    d_addr.sin_port = (*socket_by_process_id)[dest_process].second;
    string ip_address = (*socket_by_process_id)[dest_process].first;
    inet_pton(AF_INET, ip_address.c_str(), &(d_addr.sin_addr));

    // Create the ack string
    string message_to_send = to_string(ack_message);
    sendto(sockfd, message_to_send.c_str(), strlen(message_to_send.c_str()),
           0, (const struct sockaddr *) &d_addr,
           sizeof(d_addr));
}

/**
 * This method gets the incoming messages and then handles both
 * the sending of the ack and the delivery at perfect link level.
 *
 * It avoids duplicate messages.
 * @return the next new message.
 */
pp2p_message Link::get_next_message(){
    while(true){
        // Dequeue one message
        unique_lock<mutex> lck(mtx_incoming_messages);
        cv_incoming_messages.wait(lck, [&] { return !incoming_messages.empty() || sigkill; });
        if (sigkill) {
            // da_proc received stop signal
            break;   // Stop here before doing anything else
        }
        queue_incoming_messages_locked = true;
        pp2p_message msg = incoming_messages.front();
        incoming_messages.pop();
        queue_incoming_messages_locked = false;
        cv_incoming_messages.notify_one();
#ifdef DEBUG
        cout << "get next message" << endl;
#endif
        if (msg.ack) {
            // Save the ack
            mtx_acks.lock();
            acks[msg.proc_number].insert(msg.seq_number);
            mtx_acks.unlock();
        } else {
            this->send_ack(msg);
            // Check if it has not been delivered yet
            if (pl_delivered[msg.proc_number].find(msg.seq_number) == pl_delivered[msg.proc_number].end()) {
                // in case it hasn't been delivered, deliver it.
                pl_delivered[msg.proc_number].insert(msg.seq_number);
                return msg;
            }
        }
    }
    // This happens only when the process is killed, no harm can be done!
    pp2p_message stop = create_stop_pp2p_message(this->number_of_processes);
    return stop;
}


/**
 * Method run by the thread sender, it dequeues outgoing messages and then sends them
 *
 * @param socket_by_process_id  data structure that maps every process (number) to its (ip, port) pair
 * @param sockfd the socket fd of the link's owner process
 */
void run_sender(unordered_map<int, pair<string, int>>* socket_by_process_id, int sockfd) {
    struct sockaddr_in d_addr;
    while (true) {
        mtx_outgoing_messages.lock();
        if (!outgoing_messages.empty()) {
            // Dequeue one message
            pp2p_message msg = outgoing_messages.front();
            outgoing_messages.pop();
            mtx_outgoing_messages.unlock();

            mtx_acks.lock();
            if (acks[msg.dest_proc_number].find(msg.seq_number) == acks[msg.dest_proc_number].end()){
                // Send only if the ack hasn't been received
                mtx_acks.unlock();
                string msg_s = to_string(msg);
                const char *msg_c = msg_s.c_str();

                memset(&d_addr, 0, sizeof(d_addr));
                d_addr.sin_family = AF_INET;
                d_addr.sin_port = (*socket_by_process_id)[msg.dest_proc_number].second;
                string ip_address = (*socket_by_process_id)[msg.dest_proc_number].first;
                inet_pton(AF_INET, ip_address.c_str(), &(d_addr.sin_addr));

                sendto(sockfd, msg_c, strlen(msg_c),
                       0, (const struct sockaddr *) &d_addr,
                       sizeof(d_addr));
#ifdef DEBUG
                cout << "Sent " << msg_c << " to " << dest_and_msg.first << endl;
#endif
                if (!msg.ack) {
                    // If the message is not an ack, we need to resend it again in the future if the ack hasn't been received.
                    mtx_outgoing_messages.lock();
                    outgoing_messages.push(msg);
                    mtx_outgoing_messages.unlock();
                }
            } else {
                mtx_acks.unlock();
            }
        } else {
            mtx_outgoing_messages.unlock();
        }

        if (sigkill){   // sigkill is atomic, no need to manage concurrency
            break;
        }
        usleep(10);    // Wait a while, empirically proves to be better.
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

        if (sigkill){  // stop_pp2p_receiver is atomic, shouldn't need a mutex anymore
            break;
        }

        buf[n] = '\0';

        pp2p_message msg = parse_message(string(buf), link->get_process_number());
#ifdef DEBUG
        cout << "\nNew message " << msg.proc_number << " " << msg.seq_number << endl;
#endif
        // Put the message in the queue.
        unique_lock<mutex> lck(mtx_incoming_messages);
        cv_incoming_messages.wait(lck, [&] { return !queue_incoming_messages_locked; });
        queue_incoming_messages_locked = true;
        incoming_messages.push(msg);
        queue_incoming_messages_locked = false;
        cv_incoming_messages.notify_all();
    }
}
