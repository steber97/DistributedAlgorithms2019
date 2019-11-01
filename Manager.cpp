//
// Created by stefano on 01/11/19.
//

#include "Manager.h"

bool queue_locked = false;
mutex mtx;
condition_variable cv_queue;
queue<string> incoming_messages;


Manager::Manager(int process_number, unordered_map<int, pair<string, int>> &socket_by_process_id) {
    this->process_number = process_number;
    this->socket_by_process_id = socket_by_process_id;
}

void Manager::init(){
    thread t_rec(run_receiver, this->socket_by_process_id[this->process_number].first,
            this->socket_by_process_id[this->process_number].second);
    t_rec.join();
}

void send_to(int process_id, string msg){

}

void run_receiver(string ip_address, int port){
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
        //unique_lock<mutex> lck(mtx);
        //cv_queue.wait(lck, [&]{ return !queue_locked; });
        //queue_locked = true;
        //incoming_messages.push(buf);
        //queue_locked = false;
        //cv_queue.notify_one();
    }
}


