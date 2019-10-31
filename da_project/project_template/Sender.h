//
// Created by stefano on 31/10/19.
//

#ifndef PROJECT_TEMPLATE_SENDER_H
#define PROJECT_TEMPLATE_SENDER_H

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <cstring>

using namespace std;

class Sender {
    int total_number_of_messages;
    int last_sent;       // for stop-and-wait
    int s_port, d_port;
    sockaddr_in s_addr, d_addr;
    vector<bool> acks;       // Set to False, and as long as total_number_of_messages

public:
    Sender(int total_number_of_messages, int s_port, int d_port, const char* s_addr, const char* d_addr);
};


#endif //PROJECT_TEMPLATE_SENDER_H
