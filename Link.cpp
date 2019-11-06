#include "Link.h"
#include "utilities.h"
#include "TimerKiller.h"

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
    thread t_rec(run_receiver, this->socket_by_process_id[this->process_number].first,
            this->socket_by_process_id[this->process_number].second, this);
    thread t_sender(run_sender, this->socket_by_process_id);
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

        if (m.ack){
            // we received an ack;
            //timer_killer_by_process_message[m.proc_number][m.seq_number].kill();
            mtx_acks.lock();
            acks[m.proc_number][m.seq_number] = true;
            mtx_acks.unlock();
        }
        else{
            pair<string, int> dest = this->socket_by_process_id[m.proc_number];
            send_ack(m, this);
            return m;
        }

    }
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
void run_sender(unordered_map<int, pair<string, int>> socket_by_process_id){
    struct sockaddr_in d_addr;
    // Create a socket
    int sockfd;
    // Creating socket file descriptor
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        cerr << "socket creation failed but maybe recovering";
    }

    while(true){
        mtx_sender.lock();
        if (!outgoing_messages.empty()){
            pair<int,message> dest_and_msg = outgoing_messages.front();
            outgoing_messages.pop();
            mtx_sender.unlock();

            if (!acks[dest_and_msg.first][dest_and_msg.second.seq_number]) {
                // Send only if the ack hasn't been received
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
                mtx_sender.lock();
                outgoing_messages.push(dest_and_msg);
                mtx_sender.lock();
            }
        }
        else{
            mtx_sender.unlock();
        }
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
    message ack_message(true, m.seq_number, link->process_number, "");
    link->send_to(m.proc_number, ack_message);

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
void run_receiver(string ip_address, int port, Link* link){
    int sockfd;
    struct sockaddr_in sock;
    // Creating socket file descriptor
    while ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        cerr << "socket creation failed but maybe recovery";
    }
    int optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
    memset(&(sock), 0, sizeof(sock));
    sock.sin_family = AF_INET; // IPv4
    inet_pton(AF_INET, ip_address.c_str(), &(sock.sin_addr));
    sock.sin_port = port;
    while ( bind(sockfd, (const struct sockaddr *)&(sock),
              sizeof(sock)) < 0 ) {
        perror("bind failed");
        //exit(EXIT_FAILURE);
    }
    while(true){
        unsigned int len;
        char buf[1024];
        struct sockaddr_in sender_addr;
        int n = recvfrom(sockfd, (char *)buf, MAXLINE, MSG_WAITALL, ( struct sockaddr *) &sender_addr,
                         &len);
        buf[n] = '\0';
        message m = parse_message(string(buf));
        cout << "received " << buf << endl;
        // Put the message in the queue.
        unique_lock<mutex> lck(mtx_receiver);
        cv_receiver.wait(lck, [&] { return !queue_locked; });
        incoming_messages.push(m);
        cv_receiver.notify_one();
    }
}

