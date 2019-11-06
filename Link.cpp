#include "Link.h"

//to handle concurrency on the incoming_messages queue between the manager of the link and the receiver
bool queue_locked = false;
mutex mtx_receiver, mtx_sender, mtx_acks;
condition_variable cv_receiver;
queue<message> incoming_messages;

queue<pair<int,message>> outgoing_messages;

vector<vector<bool>> acks;


Link::Link(int process_number, unordered_map<int, pair<string, int>> *socket_by_process_id) {
    this->process_number = process_number;
    this->socket_by_process_id = *socket_by_process_id;
}


void Link::init(){
    // First create the socket

    cout << "Done init" << endl;
    string ip_address = this->socket_by_process_id[this->process_number].first;
    int port = this->socket_by_process_id[this->process_number].second;
    int sockfd;
    struct sockaddr_in sock;
    // Creating socket file descriptor
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        cerr << "socket creation failed but maybe recovery";
    }
    memset(&(sock), 0, sizeof(sock));
    sock.sin_family = AF_INET; // IPv4
    inet_pton(AF_INET, ip_address.c_str(), &(sock.sin_addr));
    sock.sin_port = port;
    if ( bind(sockfd, (const struct sockaddr *)&(sock),
              sizeof(sock)) < 0 ) {
        perror("bind failed");
        //exit(EXIT_FAILURE);
    }

    this->sockfd = sockfd;

    thread t_rec(run_receiver, sockfd, *this);
    thread t_sender(run_sender, this->socket_by_process_id, sockfd);
    t_rec.detach();
    t_sender.detach();
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

int Link::get_process_number() {
    return this->process_number;
}


message Link::get_next_message(){
    while(true){
        unique_lock<mutex> lck(mtx_receiver);
        cv_receiver.wait(lck, [&] { return !incoming_messages.empty(); });
        queue_locked = true;
        message m = incoming_messages.front();
        incoming_messages.pop();
        queue_locked = false;
        cv_receiver.notify_one();
        // cout << "getting next message " << m.ack << " " << m.proc_number << " " << m.seq_number << endl;
        if (m.ack){
            // we received an ack;
            //timer_killer_by_process_message[m.proc_number][m.seq_number].kill();
            cout << "Ack! " << m.proc_number << " " << m.seq_number << " by " << this->process_number << endl;
            mtx_acks.lock();
            acks[m.proc_number][m.seq_number] = true;
            mtx_acks.unlock();
        }
        else{
            cout << "send ack " << m.proc_number << " from " << this->process_number << " seq " << m.seq_number << endl;
            send_ack(m, this);
            return m;
        }
    }
}


void run_sender(unordered_map<int, pair<string, int>> socket_by_process_id, int sockfd){
    struct sockaddr_in d_addr;

    while(true){
        mtx_sender.lock();
        //cout << "outgoing messages size " << outgoing_messages.size() << endl;
        if (!outgoing_messages.empty()){
            pair<int,message> dest_and_msg = outgoing_messages.front();
            outgoing_messages.pop();
            // cout << "size of queue " << outgoing_messages.size() << endl;
            mtx_sender.unlock();

            mtx_acks.lock();
            if (!acks[dest_and_msg.first][dest_and_msg.second.seq_number]) {
                // Send only if the ack hasn't been received
                mtx_acks.unlock();
                string msg_s = create_message(dest_and_msg.second);
                const char *msg_c = msg_s.c_str();

                memset(&d_addr, 0, sizeof(d_addr));
                d_addr.sin_family = AF_INET;
                d_addr.sin_port = socket_by_process_id[dest_and_msg.first].second;
                string ip_address = socket_by_process_id[dest_and_msg.first].first;
                // wrong line here
                inet_pton(AF_INET, ip_address.c_str(), &(d_addr.sin_addr));

                sendto(sockfd, msg_c, strlen(msg_c),
                       MSG_CONFIRM, (const struct sockaddr *) &d_addr,
                       sizeof(d_addr));
                cout << "Sent " << msg_c << " to " << dest_and_msg.first << endl;
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
 * Sends ack message,
 * it is triggered whenever a normal message is received.
 * @param ip_address ip address where to send the ack
 * @param port the port where to send the acks
 * @param s_process_number process number of the source (which has to be reported in the ack message)
 * @param sequence_number the sequence number of the message we are acking.
 */
void send_ack(message m, Link* link) {
    // Create the ack string (1-source process-sequence number of message to ack)
    int source_process = link->process_number;
    int dest_process = m.proc_number;
    message ack_message(true, m.seq_number, source_process, "");
    //cout << "Sending the ack from " << source_process << " to " << dest_process << " with seq " << ack_message.seq_number << endl;

    struct sockaddr_in d_addr;

    memset(&d_addr, 0, sizeof(d_addr));
    d_addr.sin_family = AF_INET;
    d_addr.sin_port = link->socket_by_process_id[dest_process].second;
    string ip_address = link->socket_by_process_id[dest_process].first;
    // wrong line here
    inet_pton(AF_INET, ip_address.c_str(), &(d_addr.sin_addr));
    string message_to_send = create_message(ack_message);
    sendto(link->sockfd, message_to_send.c_str(), strlen(message_to_send.c_str()),
           MSG_CONFIRM, (const struct sockaddr *) &d_addr,
           sizeof(d_addr));
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

void run_receiver(int sockfd, Link link) {


    while(true){
        unsigned int len;
        char buf[1024];
        struct sockaddr_in sender_addr;
        int n = recvfrom(sockfd, (char *)buf, MAXLINE, MSG_WAITALL, ( struct sockaddr *) &sender_addr,
                         &len);
        buf[n] = '\0';
        message m = parse_message(string(buf));
        cout << "received " << buf << " by " << link.process_number << " lol" << endl;
        // Put the message in the queue.
        unique_lock<mutex> lck(mtx_receiver);
        cv_receiver.wait(lck, [&] { return !queue_locked; });
        incoming_messages.push(m);
        cv_receiver.notify_one();
    }
}

