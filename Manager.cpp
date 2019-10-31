//
// Created by stefano on 31/10/19.
//

#include <iostream>
#include <pthread.h>

#include "Manager.h"
using namespace std;


struct thread_data{
    int total_number_of_messages;
    const char* destination_addr;
    int d_port;
};

void run_receiver(char* receiver_addr, int s_port){
    int sockfd;
    struct sockaddr_in s_addr, d_addr;

    // Creating socket file descriptor
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        cerr << "socket creation failed";
        exit(EXIT_FAILURE);
    }
    memset(&(s_addr), 0, sizeof(s_addr));
    s_addr.sin_family = AF_INET; // IPv4
    inet_pton(AF_INET, receiver_addr, &(s_addr.sin_addr));
    //this->servaddr.sin_addr = INADDR_ANY;
    s_addr.sin_port = htons(s_port);

    if ( bind(sockfd, (const struct sockaddr *)&(s_addr),
              sizeof(s_addr)) < 0 ) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }


    // TODO: while loop and receive messages

}


void run_sender(int total_number_of_messages, const char* destination_addr, int d_port){
    cout << "Create a new sender" << endl;

    struct sockaddr_in d_addr;
    int last_ack = 0;

    // Messages start from 1. We always send the next message to send (last_send + 1)
    int last_sent = 0;

    // Create a socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    memset(&d_addr, 0, sizeof(d_addr));
    d_addr.sin_family = AF_INET;
    d_addr.sin_addr.s_addr = INADDR_ANY;
    d_addr.sin_port = htons(d_port);

    

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

    for (int i = 0; i<this->processes.size(); i++){
        cout << "Create thread" << endl;
        if (i+1 != this->process_number){
            thread_data td;
            td.d_port = this->ports[i];
            td.destination_addr = this->ips[i];
            td.total_number_of_messages = this->number_of_messages;
            pthread_create(&senders[i], NULL, reinterpret_cast<void *(*)(void *)>(run_sender), (void *)&td);
        }
    }

    t_rec.join();
    for (pthread_t t: senders){
        pthread_join(t, &status);
    }
}