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
#define MAX_NUMBER_OF_PROCESSES 100
#define SLOW_START 0
#define CONGESTION_AVOIDANCE 1
#define FAST_RECOVERY 2

using namespace std;

/// To handle the size of the congestion
vector<int> state;
vector<int> congestion_window_size;
vector<int> ssthresh;
vector<int> duplicate_ack_count;
vector<float> congestion_avoidance_augment;
vector<mutex> mtx_state(MAX_NUMBER_OF_PROCESSES);
vector<mutex> mtx_congestion_window_size(MAX_NUMBER_OF_PROCESSES);
vector<mutex> mtx_ssthresh(MAX_NUMBER_OF_PROCESSES);
vector<mutex> mtx_duplicate_ack_count(MAX_NUMBER_OF_PROCESSES);


/// These variables are useful to handle concurrency on data structure accessed by the threads sender and receiver
extern mutex mtx_incoming_messages, mtx_acks;
extern condition_variable cv_incoming_messages;
extern queue<pp2p_message> incoming_messages;

extern vector<mutex> mtx_messages_to_send_by_process;
extern vector<queue<pp2p_message>> messages_to_send_by_process;

extern mutex mtx_outgoing_messages;
extern queue<pair<int,pp2p_message>> outgoing_messages;

// Both acks and pl_delivered are indexed in this way:
//  - the vector is indexed by the process number in the perfect link message
//  - the set contains the sequence numbers of the messages in it
extern vector<unordered_set<long long int>> acks;
extern vector<unordered_set<long long int>> pl_delivered;  // messages delivered by perfect link.


/**
 * Run sender: it is run by a single thread,
 * creates a socket which will periodically try to send a message to
 * the desired ip_address and port.
 * Whenever the ack is received, the receiver notify the
 * timer_killer object, which stops the sender thread
 * @param socket_by_process_id the map from process number to destination address.
 */
void run_sender(unordered_map<int, pair<string, int>>* socket_by_process_id, int sockfd);


/**
 * Class representing a perfect link
 */
class Link {
private:
    int sockfd;
    int process_number;
    unordered_map<int, pair<string, int>>* socket_by_process_id;
    vector<long long> last_seq_number;

public:
    Link(int sockfd, int process_number, unordered_map<int, pair<string, int>> *socket_by_process_id);
    void init();
    int get_sockfd();
    int get_process_number();
    void send_to(int d_process_number, pp2p_message& msg);
    void send_ack(pp2p_message &msg);
    pp2p_message get_next_message();

    void on_new_ack(int proc_num);
    void on_duplicate_ack(int proc_num);
    void on_timeout(int proc_num);
    void enqueue_messages(int proc_num, int number_of_messages);
};

void run_receiver(Link *link);

int get_state(int proc_num) {
    mtx_state[proc_num - 1].lock();
    int s = state[proc_num - 1];
    mtx_state[proc_num - 1].unlock();
    return s;
}

int get_congestion_window_size(int proc_num) {
    mtx_congestion_window_size[proc_num - 1].lock();
    int size = congestion_window_size[proc_num - 1];
    mtx_congestion_window_size[proc_num - 1].unlock();
    return size;
}

int get_ssthresh(int proc_num) {
    mtx_ssthresh[proc_num - 1].lock();
    int size = ssthresh[proc_num - 1];
    mtx_ssthresh[proc_num - 1].unlock();
    return size;
}

int get_duplicate_ack_count(int proc_num) {
    mtx_duplicate_ack_count[proc_num - 1].lock();
    int count = duplicate_ack_count[proc_num - 1];
    mtx_duplicate_ack_count[proc_num - 1].unlock();
    return count;
}

void set_state(int proc_num, int new_value) {
    mtx_state[proc_num - 1].lock();
    state[proc_num - 1] = new_value;
    mtx_state[proc_num - 1].unlock();
}

/**
 * Setter for congestion window's size
 * @param proc_num the process of which the window should be set to the desired value
 * @param new_value the value to which set the window size
 * @return the number of messages to send to the process <proc_num>
 */
int set_congestion_window_size(int proc_num, int new_value) {
    mtx_congestion_window_size[proc_num - 1].lock();
    int old_value = congestion_window_size[proc_num - 1];
    congestion_window_size[proc_num - 1] = new_value;
    mtx_congestion_window_size[proc_num - 1].unlock();
    return max(0, new_value - old_value);
}

/**
 * Increases congestion window's size according to the parameter state
 * @param proc_num the process of which the window should be increased
 * @param st the state of the connection with the process <proc_num>
 * @return the number of messages to send to the process <proc_num>
 */
int increase_congestion_window_size(int proc_num, int st) {
    bool size_incremented = false;
    mtx_congestion_window_size[proc_num - 1].lock();
    if (st == SLOW_START) {
        congestion_window_size[proc_num - 1]++;
        size_incremented = true;
    } else if (st == CONGESTION_AVOIDANCE) {
        congestion_avoidance_augment[proc_num - 1] += 1.0 / float(congestion_window_size[proc_num - 1]);
    }
    mtx_congestion_window_size[proc_num - 1].unlock();

    if (congestion_avoidance_augment[proc_num - 1] == 1.0)
        size_incremented = true;

    return size_incremented ? 1 : 0;
}

void set_ssthresh(int proc_num, int new_value) {
    mtx_ssthresh[proc_num - 1].lock();
    ssthresh[proc_num - 1] = new_value;
    mtx_ssthresh[proc_num - 1].unlock();
}

void reset_duplicate_ack_count(int proc_num) {
    mtx_duplicate_ack_count[proc_num - 1].lock();
    duplicate_ack_count[proc_num - 1] = 0;
    mtx_duplicate_ack_count[proc_num - 1].unlock();
}

void increase_duplicate_ack_count(int proc_num) {
    mtx_duplicate_ack_count[proc_num - 1].lock();
    duplicate_ack_count[proc_num - 1]++;
    mtx_duplicate_ack_count[proc_num - 1].unlock();
}

#endif //DISTRIBUTEDALGORITHMS2019_MANAGER_H
