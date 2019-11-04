#include "Link.h"
#include "utilities.h"
#include "ThreadKill.h"

//to handle concurrency on the incoming_messages queue between the manager of the link and the receiver
bool queue_locked = false;
mutex mtx_receiver;
condition_variable cv_receiver;
queue<message> incoming_messages;


Link::Link(int process_number, unordered_map<int, pair<string, int>> *socket_by_process_id) {
    this->process_number = process_number;
    this->socket_by_process_id = *socket_by_process_id;
}


void Link::init(){
    thread t_rec(run_receiver, this->socket_by_process_id[this->process_number].first,
            this->socket_by_process_id[this->process_number].second, this);
    t_rec.detach();
}


/**
 * Format of messages:
 * - 1 bit for ack: 0 normal message, 1 ack.
 * @param d_process_number destination address.
 * @param msg the message to send
 * @param sequence_number the sequence number of the message.
 */
void Link::send_to(int d_process_number, message& msg, int sequence_number) {

    string message =
            (msg.ack ? string("1") : string("0")) + "-" + to_string(msg.proc_number) + "-" + to_string(msg.seq_number);
    thread t_rec(run_sender, message, this->socket_by_process_id[d_process_number].first,
                 this->socket_by_process_id[d_process_number].second, d_process_number,
                 sequence_number); //, this->socket_by_process_id[d_process_number].first);
}

int Link::get_process_number() {
    return this->process_number;
}


string Link::get_next_message(){
    while(true){
        cout << "waiting for next message" << endl;
        unique_lock<mutex> lck(mtx_receiver);
        cv_receiver.wait(lck, [&] { return !incoming_messages.empty(); });
        queue_locked = true;
        message m = incoming_messages.front();
        incoming_messages.pop();
        queue_locked = false;
        cv_receiver.notify_one();


        if (m.ack){
            // we received an ack;
            process_message_thread_kill[m.proc_number][m.seq_number].kill();
            cout << "We received an ack for message " << m.seq_number << " by " << m.proc_number << endl;
        }
        else{
            cout << "We received message " << m.seq_number << " from " << m.proc_number << endl;
            pair<string, int> dest = this->socket_by_process_id[m.proc_number];
            send_ack(dest.first, dest.second, this->process_number, m.seq_number);
            // Beb delivery
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
void run_sender(string msg, string ip_address, int port, int destination_process, int sequence_number){
    struct sockaddr_in d_addr;
    // Create a socket
    int sockfd;
    // Creating socket file descriptor
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        cerr << "socket creation failed";
        exit(EXIT_FAILURE);
    }
    memset(&d_addr, 0, sizeof(d_addr));
    d_addr.sin_family = AF_INET;
    d_addr.sin_port = port;
    // wrong line here
    inet_pton(AF_INET, ip_address.c_str(), &(d_addr.sin_addr));


    bool got_killed = true;
    cout << "Try to send again message " << sequence_number << " to process " << destination_process
            << " at address " << ip_address << endl;
    while(got_killed){
        const char* message = msg.c_str();
        sendto(sockfd, message, strlen(message),
               MSG_CONFIRM, (const struct sockaddr *) &d_addr,
               sizeof(d_addr));
        got_killed = process_message_thread_kill[destination_process][sequence_number].wait_for(std::chrono::milliseconds(1000));
    }
    cout << "Received ack, stop sending! :)" << endl;
}

/**
 * Sends ack message,
 * it is triggered whenever a normal message is received.
 * @param ip_address ip address where to send the ack
 * @param port the port where to send the acks
 * @param s_process_number process number of the source (which has to be reported in the ack message)
 * @param sequence_number the sequence number of the message we are acking.
 */
void send_ack(string ip_address, int port, int s_process_number, int sequence_number) {
    struct sockaddr_in d_addr;
    // Create a socket
    int sockfd;
    // Creating socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        cerr << "socket creation failed";
        exit(EXIT_FAILURE);
    }
    memset(&d_addr, 0, sizeof(d_addr));
    d_addr.sin_family = AF_INET;
    d_addr.sin_port = port;
    // wrong line here
    inet_pton(AF_INET, ip_address.c_str(), &(d_addr.sin_addr));

    // Create the ack string (1-source process-sequence number of message to ack)
    string msg = "1-" + to_string(s_process_number) + "-" + to_string(sequence_number);

    const char *message = msg.c_str();
    sendto(sockfd, message, strlen(message),
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
void run_receiver(string ip_address, int port, Link* link){
    int sockfd;
    struct sockaddr_in sock;
    // Creating socket file descriptor
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        cerr << "socket creation failed";
        exit(EXIT_FAILURE);
    }
    memset(&(sock), 0, sizeof(sock));
    sock.sin_family = AF_INET; // IPv4
    inet_pton(AF_INET, ip_address.c_str(), &(sock.sin_addr));
    sock.sin_port = port;
    if ( bind(sockfd, (const struct sockaddr *)&(sock),
              sizeof(sock)) < 0 ) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    while(true){
        unsigned int len;
        char buf[1024];
        struct sockaddr_in sender_addr;
        int n = recvfrom(sockfd, (char *)buf, MAXLINE, MSG_WAITALL, ( struct sockaddr *) &sender_addr,
                         &len);
        buf[n] = '\0';
        cout << "received " << buf << endl;

        message m;
        m = parse_message(string(buf));

        // Put the message in the queue.
        unique_lock<mutex> lck(mtx_receiver);
        cv_receiver.wait(lck, [&] { return !queue_locked; });
        incoming_messages.push(m);
        cv_receiver.notify_one();
    }
}

