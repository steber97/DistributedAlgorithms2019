#include "Link.h"
#include "utilities.h"

//to handle concurrency on the incoming_messages queue between the manager of the link and the receiver
bool queue_locked = false;
mutex mtx_receiver;
condition_variable cv_receiver;
queue<string> incoming_messages;

//to handle concurrency on the acks between the manager of the link and the sender
vector<vector<bool>> acks;
mutex mtx_sender;
condition_variable cv_sender;


Link::Link(int process_number, unordered_map<int, pair<string, int>> *socket_by_process_id) {
    this->process_number = process_number;
    this->socket_by_process_id = *socket_by_process_id;
}


void Link::init(){
    thread t_rec(run_receiver, this->socket_by_process_id[this->process_number].first,
            this->socket_by_process_id[this->process_number].second);
    t_rec.detach();
}


void Link::send_to(int d_process_number, string &msg){
    thread t_rec(run_sender, msg, this->socket_by_process_id[d_process_number].first,
            this->socket_by_process_id[d_process_number].second);
    t_rec.detach();
}


string Link::get_next_message(){
    while(true){
        unique_lock<mutex> lck(mtx_receiver);
        cv_receiver.wait(lck, [&] { return !incoming_messages.empty(); });
        queue_locked = true;
        string message = incoming_messages.front();
        incoming_messages.pop();
        queue_locked = false;
        cv_receiver.notify_one();
        if(is_ack(message))
            ack_received(message);
        else
            return message;
    }
}


void run_sender(string &msg, string &ip_address, int port){
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
    unique_lock<mutex> lck(mtx_sender);

    //TODO attenzione alla concorrenza
    while(/*ack not received*/){
        const char* message = msg.c_str();
        sendto(sockfd, message, strlen(message),
               MSG_CONFIRM, (const struct sockaddr *) &d_addr,
               sizeof(d_addr));
        cv_sender.wait_for(lck, chrono::milliseconds(10), [&]{ return acks[/*right ack*/]});
    }
}


void run_receiver(string &ip_address, int port){
    /**
     * Creates a socket to receive incoming messages
     */
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
        cout << "Waiting for message" << endl;
        unsigned int len;
        char buf[1024];
        struct sockaddr_in sender_addr;
        int n = recvfrom(sockfd, (char *)buf, MAXLINE, MSG_WAITALL, ( struct sockaddr *) &sender_addr,
                         &len);
        buf[n] = '\0';
        cout << "received " << buf << endl;
        unique_lock<mutex> lck(mtx_receiver);
        cv_receiver.wait(lck, [&]{ return !queue_locked; });
        queue_locked = true;
        incoming_messages.push(buf);
        queue_locked = false;
        cv_receiver.notify_one();
    }
}


