#ifndef DISTRIBUTEDALGORITHMS2019_MANAGER_H
#define DISTRIBUTEDALGORITHMS2019_MANAGER_H

#include <cstdio>
#include <cstdlib>
#include <csignal>
#include <ctime>
#include <unistd.h>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <vector>
#include <queue>
#include <unordered_map>
#include <mutex>
#include <iostream>
#include <pthread.h>
#include <thread>
#include <condition_variable>

#include "utilities.h"

#define MAXLINE 1024

using namespace std;

extern struct sockaddr_in sock;

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
void run_sender(string msg, string ip_address, int port, int destination_process, int sequence_number, int sockfd);

/**
 * Sends ack message,
 * it is triggered whenever a normal message is received.
 * @param ip_address ip address where to send the ack
 * @param port the port where to send the acks
 * @param s_process_number process number of the source (which has to be reported in the ack message)
 * @param sequence_number the sequence number of the message we are acking.
 */
void send_ack(string ip_address, int port, int s_process_number, int sequence_number, int sockfd);

class Link {

public:
    Link(int process_number, unordered_map<int, pair<string, int>> *socket_by_process_id);
    void init(int sockfd);
    int get_process_number();
    message get_next_message();
    void send_to(int d_process_number, message& msg, int sockfd);
    unordered_map<int, pair<string, int>> socket_by_process_id;
    int process_number;
    int sockfd;
};


void run_receiver(string ip_address, int port, Link *link, int sockfd);

int setup_socket(Link* link);

#endif //DISTRIBUTEDALGORITHMS2019_MANAGER_H
