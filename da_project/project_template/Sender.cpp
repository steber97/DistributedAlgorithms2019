//
// Created by stefano on 31/10/19.
//

#include <iostream>
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "Sender.h"

using namespace std;

Sender::Sender(int total_number_of_messages, int s_port, int d_port, const char* s_addr, const char* d_addr) {
    /**
     * This is the sender class.
     * It stores the number of messges to send, the source and destination port and address.
     * Moreover, it keeps in a variable the last message sent (starts from 0)
     * Every new message sent is last_sent + 1, and last_sent is the last for which an ack has been received.
     */
    cout << "Create a new sender" << endl;
    this->total_number_of_messages = total_number_of_messages;
    this->s_port = s_port;
    this->d_port = d_port;

    this->acks.resize(total_number_of_messages, false);

    // Messages start from 1. We always send the next message to send (last_send + 1)
    this->last_sent = 0;

    // Create a socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    memset(&this->d_addr, 0, sizeof(this->d_addr));
    this->d_addr.sin_family = AF_INET;
    this->d_addr.sin_port = htons(this->d_port);
    inet_pton(AF_INET, d_addr, &this->d_addr.sin_addr);
}