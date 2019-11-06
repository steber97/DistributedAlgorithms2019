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
void run_sender(unordered_map<int, pair<string, int>>* socket_by_process_id, int sockfd);

class Link {
private:
    int sockfd;
    int process_number;
    unordered_map<int, pair<string, int>>* socket_by_process_id;
public:
    Link(int sockfd, int process_number, unordered_map<int, pair<string, int>> *socket_by_process_id);
    void init();
    int get_sockfd();
    int get_process_number();
    void send_to(int d_process_number, message& msg);
    void send_ack(message msg);
    message get_next_message();
    void pp2p_deliver(message msg);
};

void run_receiver(Link *link);

#endif //DISTRIBUTEDALGORITHMS2019_MANAGER_H
