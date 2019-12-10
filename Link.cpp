#include "Link.h"

//to handle concurrency on the incoming_messages queue between the manager of the link and the receiver
bool queue_locked = false;
mutex mtx_receiver, mtx_sender, mtx_acks;
condition_variable cv_receiver;

//These two data structure contain the incoming and the outgoing messages
queue<pp2p_message> incoming_messages;
queue<pair<int,pp2p_message>> outgoing_messages;

vector<unordered_set<long long int>> acks;  // acks
vector<unordered_set<long long int>> pl_delivered;  //sequence numbers of the delivered messages at perfect link level
                                                    //ordered by the process that sent them


Link::Link(int sockfd, int process_number, unordered_map<int, pair<string, int>> *socket_by_process_id, const int number_of_processes) {
    // Constructor
    this->sockfd = sockfd;
    this->process_number = process_number;
    this->socket_by_process_id = socket_by_process_id;
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
    mtx_sender.lock();
    outgoing_messages.push({d_process_number, msg});
    mtx_sender.unlock();
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
    // TODO: Check this while loop
    while(true){
        unique_lock<mutex> lck(mtx_receiver);
        cv_receiver.wait(lck, [&] { return !incoming_messages.empty(); });
        queue_locked = true;
        pp2p_message msg = incoming_messages.front();
        incoming_messages.pop();
        queue_locked = false;
        cv_receiver.notify_one();
#ifdef DEBUG
        cout << "get next message" << endl;
#endif
        if (msg.ack) {
            // we received an ack;
            // cout << "Received ack :) " << msg.proc_number << " " << msg.seq_number << endl;
            mtx_acks.lock();
            acks[msg.proc_number].insert(msg.seq_number);
            mtx_acks.unlock();
        } else {
            this->send_ack(msg);
            // Check if it has not been delivered already
            if (pl_delivered[msg.proc_number].find(msg.seq_number) == pl_delivered[msg.proc_number].end()) {
                pl_delivered[msg.proc_number].insert(msg.seq_number);
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
    pp2p_message fake = create_fake_pp2p(this->number_of_processes);
    return fake;
}


/**
 * Method run by the thread sender, it dequeues outgoing messages and then sends
 *
 * @param socket_by_process_id  data structure that maps every process (number) to its (ip, port) pair
 * @param sockfd the socket fd of the link's owner process
 */
void run_sender(unordered_map<int, pair<string, int>>* socket_by_process_id, int sockfd) {
    struct sockaddr_in d_addr;
    while (true) {
        mtx_sender.lock();
        if (!outgoing_messages.empty()) {
            pair<int, pp2p_message> dest_and_msg = outgoing_messages.front();
            outgoing_messages.pop();
            mtx_sender.unlock();

            mtx_acks.lock();
            if (acks[dest_and_msg.first].find(dest_and_msg.second.seq_number) == acks[dest_and_msg.first].end()){
                // Send only if the ack hasn't been received
                mtx_acks.unlock();
                string msg_s = to_string(dest_and_msg.second);
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
                    mtx_sender.lock();
                    outgoing_messages.push(dest_and_msg);
                    mtx_sender.unlock();
                }
            } else {
                mtx_acks.unlock();
            }
        } else {
            mtx_sender.unlock();
        }
        // wait a bit before sending the new message.
        usleep(10);
        mtx_pp2p_sender.lock();
        if (stop_pp2p_sender){
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

        if (stop_pp2p_receiver){  // stop_pp2p_receiver is atomic, shouldn't need a mutex anymore
            break;
        }

        buf[n] = '\0';

        pp2p_message msg = parse_message(string(buf));
#ifdef DEBUG
        cout << "\nNew message " << msg.proc_number << " " << msg.seq_number << endl;
#endif
        // Put the message in the queue.
        unique_lock<mutex> lck(mtx_receiver);
        cv_receiver.wait(lck, [&] { return !queue_locked; });
        queue_locked = true;
        incoming_messages.push(msg);
        queue_locked = false;
        cv_receiver.notify_one();
    }
}
