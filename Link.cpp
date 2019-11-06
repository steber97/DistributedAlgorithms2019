#include "Link.h"

//to handle concurrency on the incoming_messages queue between the manager of the link and the receiver
bool queue_locked = false;
mutex mtx_receiver, mtx_sender, mtx_acks;
condition_variable cv_receiver;
queue<message> incoming_messages;

queue<pair<int,message>> outgoing_messages;

vector<vector<bool>> acks;


Link::Link(int sockfd, int process_number, unordered_map<int, pair<string, int>> *socket_by_process_id) {
    this->sockfd = sockfd;
    this->process_number = process_number;
    this->socket_by_process_id = socket_by_process_id;
}


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
 * This method only puts the message in the queue of outgoing messages, so that the sender can send it.
 * @param d_process_number destination address.
 * @param msg the message to send
 * @param sequence_number the sequence number of the message.
 */
void Link::send_to(int d_process_number, message& msg) {
    // At the moment, we keep a queue of messages to send and not acked yet.
    // Sending a message consist easily in putting a new message in the queue, and
    // wait until the run_sender method sends it.
    mtx_sender.lock();
    outgoing_messages.push({d_process_number, msg});
    mtx_sender.unlock();
}


void Link::send_ack(message msg) {
    // Create the ack string (1-source process-sequence number of message to ack)
    int source_process = process_number;
    int dest_process = msg.proc_number;
    message ack_message(true, msg.seq_number, source_process, "");
    cout << "Sending the ack from " << source_process << " to " << dest_process << " with seq " << ack_message.seq_number << endl;

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


message Link::get_next_message(){
    while(true){
        cout << "waiting for next message" << endl;
        unique_lock<mutex> lck(mtx_receiver);
        //TODO correggere errore sulla concorrenza
        cv_receiver.wait(lck, [&] { return !incoming_messages.empty(); });
        queue_locked = true;
        message msg = incoming_messages.front();
        incoming_messages.pop();
        queue_locked = false;
        cv_receiver.notify_one();
        return msg;
    }
}


void Link::pp2p_deliver(message msg){
    cout << "pp2p delivery di: " << msg.seq_number << " da " << msg.proc_number;
}

/**
 * Run sender: it is run by a single thread,
 * creates a socket which will periodically try to send a message to
 * the desired ip_address and port.
 * Whenever the ack is received, the receiver notify the
 * timer_killer object, which stops the sender thread
 * @param msg the message to send, already converted to string.
 * @param ip_address the destination ip address
 * @param port the destination port
 * @param destination_process the destination process number (it is used to
 *              receive the notification by the receiver about the ack receival)
 * @param sequence_number the sequence number of the message.
 */
/**

 */
void run_sender(unordered_map<int, pair<string, int>>* socket_by_process_id, int sockfd){
    //TODO rivedere concorrenza qui
    struct sockaddr_in d_addr;
    while(true){
        mtx_sender.lock();
        if (!outgoing_messages.empty()){
            pair<int,message> dest_and_msg = outgoing_messages.front();
            outgoing_messages.pop();
            // cout << "size of queue " << outgoing_messages.size() << endl;
            mtx_sender.unlock();

            mtx_acks.lock();
            if (!acks[dest_and_msg.first][dest_and_msg.second.seq_number]) {
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
                // cout << "Sent " << msg_c << " to " << dest_and_msg.first << endl;
                if (!dest_and_msg.second.ack){
                    mtx_sender.lock();
                    outgoing_messages.push(dest_and_msg);
                    mtx_sender.unlock();
                }
            }
            else{
                mtx_acks.unlock();
            }
        }
        else{
            mtx_sender.unlock();
        }
        // wait a bit before sending the new message.
        usleep(1000);
    }
    //cout << "Received ack for " << sequence_number << ", stop sending! :)" << endl;
}


/**
 * This is the receiver:
 * it starts listening all messages coming to the desired ip_address and port
 * (the address and port of the actual process).
 * Whenever it receives a message, it puts messages in a shared queue
 * with the Link, which will manage to process the data according to
 * the needs (acks or messages)
 * @param ip_address the ip address the receiver is linked to
 * @param port  the receiver port
 * @param link  the link object
 */
void run_receiver(Link *link) {
    while(true){
        unsigned int len;
        char buf[1024];
        struct sockaddr_in sender_addr;
        int n = recvfrom(link->get_sockfd(), (char *)buf, MAXLINE, MSG_WAITALL, ( struct sockaddr *) &sender_addr,
                         &len);
        buf[n] = '\0';
        cout << "received " << buf << endl;
        message msg;
        msg = parse_message(string(buf));

        cout << "\nNew message: " << msg.seq_number << "\tfrom " << msg.proc_number;

        // Put the message in the queue.
        unique_lock<mutex> lck(mtx_receiver);
        cv_receiver.wait(lck, [&] { return !queue_locked; });
        queue_locked = true;
        incoming_messages.push(msg);
        queue_locked = false;
        cv_receiver.notify_one();
    }
}




