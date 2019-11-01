//
// Created by stefano on 31/10/19.
//

#include <iostream>
#include <pthread.h>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>

#include "Manager.h"


#define MAXLINE 1024

using namespace std;

struct message{
    string content;
    struct sockaddr_in addr;
    message(string content, struct sockaddr_in addr) : content{content}, addr{addr} {};
};

bool queue_locked = false;

mutex mtx;

condition_variable cv_queue;
queue<message> queue_receiver_master;


struct thread_data{
    int total_number_of_messages;
    const char* destination_addr;
    int d_port;
    thread_data(int total_number_of_messages, char* destination_addr, int d_port ):
        total_number_of_messages{total_number_of_messages}, destination_addr {destination_addr}, d_port {d_port} {};
};

void run_receiver(char* receiver_addr, int s_port){
    int sockfd;
    struct sockaddr_in recv_addr, sender_addr;

    // Creating socket file descriptor
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        cerr << "socket creation failed";
        exit(EXIT_FAILURE);
    }
    memset(&(recv_addr), 0, sizeof(recv_addr));
    recv_addr.sin_family = AF_INET; // IPv4
    inet_pton(AF_INET, receiver_addr, &(recv_addr.sin_addr));
    //this->servaddr.sin_addr = INADDR_ANY;
    recv_addr.sin_port = htons(s_port);

    if ( bind(sockfd, (const struct sockaddr *)&(recv_addr),
              sizeof(recv_addr)) < 0 ) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }


    // TODO: while loop and receive messages
    while(true){
        unsigned int len;
        char buf[1024];
        cout << "Receiver waiting for incoming messages" << endl;
        int n = recvfrom(sockfd, (char *)buf, MAXLINE, MSG_WAITALL, ( struct sockaddr *) &sender_addr,
                         &len);
        buf[n] = '\0';

        message m{buf, sender_addr};

        unique_lock<mutex> lck(mtx);
        cv_queue.wait(lck, [&]{ return !queue_locked; });
        queue_locked = true;
        cout << "Ciao" << endl;

        queue_receiver_master.push(m);
        queue_locked = false;
        cv_queue.notify_one();
    }

}


void* run_sender(void *threadarg){
    struct thread_data *my_data;
    my_data = (struct thread_data *) threadarg;
    cout << "Create a new sender on port " << my_data->d_port << endl;
    int d_port = my_data->d_port;
    int total_number_of_messages = my_data->total_number_of_messages;
    const char* destination_addr = my_data->destination_addr;

    struct sockaddr_in d_addr;
    int last_ack = 0;

    // Messages start from 1. We always send the next message to send (last_send + 1)
    int last_sent = 1;

    // Create a socket
    int sockfd;
    // Creating socket file descriptor
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        cerr << "socket creation failed";
        exit(EXIT_FAILURE);
    }
    cout << "ciao" << endl;
    memset(&d_addr, 0, sizeof(d_addr));
    cout << "ciao " << d_port << " " << destination_addr << " " << total_number_of_messages << endl;
    d_addr.sin_family = AF_INET;
    d_addr.sin_port = htonl(d_port);
    // wrong line here
    cout << destination_addr << endl;
    inet_pton(AF_INET, destination_addr, &(d_addr.sin_addr));

    while(last_ack <= total_number_of_messages){
        string message = "0" + to_string(last_sent);
        const char* msg = message.c_str();
        sendto(sockfd, msg, strlen(msg),
               MSG_CONFIRM, (const struct sockaddr *) &d_addr,
               sizeof(d_addr));

    }


}

Manager::Manager(vector<int> &ports, vector<char *> &ips, vector<int> &processes, int process_number, int number_of_messages){
    // spawns the threads
    cout << "Creating the manager" << endl;
    this->ports = ports;
    this->ips = ips;
    this->processes = processes;
    this->process_number = process_number;
    this->number_of_messages = number_of_messages;
}


void Manager::run(){
    /**
     * invokes receiver and sender runs with threads.
     */

    thread t_rec(run_receiver, this->ips[this->process_number-1], this->ports[this->process_number-1]);
    pthread_t senders[this->processes.size()];
    void *status;

    vector<thread_data> td;

    for (int i = 0; i<this->processes.size(); i++){
        cout << "Create thread " << this->process_number << " " << this->number_of_messages << endl;
        if (i+1 != this->process_number){
            cout << this->ports[i] << endl;
            thread_data temp{ this->number_of_messages, this->ips[i],this->ports[i]};
            td.push_back(temp);
            //td[i] = temp;
            pthread_create(&senders[i], NULL, run_sender, (void *)&td[i]);
        }
        else{
            // the process itself.
            td.push_back({ this->number_of_messages, this->ips[i],this->ports[i]});
        }
    }

    while(true){
        // Waiting for the queue from the receiver to be filled.
        unique_lock<mutex> lck(mtx);
        cv_queue.wait(lck, [&]{return !queue_receiver_master.empty(); });
        queue_locked = true;
        message m = queue_receiver_master.front();
        queue_receiver_master.pop();
        cout << m.content << endl;
        queue_locked = false;
        cv_queue.notify_one();
    }

    t_rec.join();
    for (pthread_t t: senders){
        pthread_join(t, &status);
    }
}