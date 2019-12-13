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
#include <unordered_set>

#include "utilities.h"

// #define DEBUG

#define MAXLINE 1024

using namespace std;

/// These variables are useful to handle concurrency on data structure accessed by the threads sender and receiver
extern mutex mtx_incoming_messages, mtx_outgoing_messages, mtx_acks;
extern condition_variable cv_incoming_messages;
extern queue<pp2p_message> incoming_messages;

/// Queue of messages that have to be sent
extern queue<pp2p_message> outgoing_messages;

/// Both acks and pl_delivered are indexed in this way:
// the vector is indexed by the process number in the perfect link message
// and the set by the sequence number of the pp2p message.

extern vector<unordered_set<long long int>> acks;  // acks at pp2p level

extern vector<unordered_set<long long int>> pl_delivered;  // Delivered by perfect link.


/**
 * Method run by the thread sender, it dequeues outgoing messages and then sends them
 *
 * @param socket_by_process_id  data structure that maps every process (number) to its (ip, port) pair
 * @param sockfd the socket fd of the link's owner process
 */
void run_sender(unordered_map<int, pair<string, int>>* socket_by_process_id, int sockfd);


/**
 * Class representing a perfect link
 */
class Link {
private:
    int sockfd;
    int process_number;
    int number_of_processes;
    unordered_map<int, pair<string, int>>* socket_by_process_id;
    vector<long long> last_seq_number;   // Last sequence number used per every message, indexed by process number.
public:
    Link(int sockfd, int process_number, unordered_map<int, pair<string, int>> *socket_by_process_id, const int number_of_processes);
    void init();
    int get_sockfd();
    int get_process_number();
    void send_to(int d_process_number, pp2p_message& msg);
    void send_ack(pp2p_message &msg);
    pp2p_message get_next_message();
};


/**
 * This is the method run by the thread receiver, it listens on a certain port until it gets a message and then it
 * puts the message the incoming_messages queue
 *
 * @param link
 */
void run_receiver(Link *link);

#endif //DISTRIBUTEDALGORITHMS2019_MANAGER_H
