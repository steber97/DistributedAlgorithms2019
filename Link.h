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
#include "TimerKiller.h"


#define MAXLINE 1024

using namespace std;


extern mutex mtx_receiver, mtx_sender, mtx_acks;
extern condition_variable cv_receiver;
extern queue<message> incoming_messages;

extern queue<pair<int,message>> outgoing_messages;

extern vector<vector<bool>> acks;


/**
 * Run sender: it is run by a single thread,
 * creates a socket which will periodically try to send a message to
 * the desired ip_address and port.
 * Whenever the ack is received, the receiver notify the
 * timer_killer object, which stops the sender thread
 * @param socket_by_process_id the map from process number to destination address.
 */
void run_sender(unordered_map<int, pair<string, int>> socket_by_process_id, int sockfd);


extern vector<vector<bool>> acks;

class Link {

public:
    Link(int process_number, unordered_map<int, pair<string, int>> *socket_by_process_id);
    void init();
    int get_process_number();
    message get_next_message();
    void send_to(int d_process_number, message& msg);
    unordered_map<int, pair<string, int>> socket_by_process_id;
    int process_number;
    int sockfd;     // TODO: see where to initialize it properly, now it is in link->init().
};

/**
 * Sends ack message,
 * it is triggered whenever a normal message is received.
 * @param ip_address ip address where to send the ack
 * @param port the port where to send the acks
 * @param s_process_number process number of the source (which has to be reported in the ack message)
 * @param sequence_number the sequence number of the message we are acking.
 */
void send_ack(message m, Link* link);

void run_receiver(int sockfd, Link link);

#endif //DISTRIBUTEDALGORITHMS2019_MANAGER_H
