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
#include <chrono>

#include "utilities.h"

//#define DEBUG

#define MAXLINE 1024
#define MAX_NUMBER_OF_PROCESSES 100
#define SLOW_START 0
#define CONGESTION_AVOIDANCE 1
#define FAST_RECOVERY 2
#define DEFAULT_TIMEOUT 1000
#define SMOOTHNESS 0.125

using namespace std;

/// To handle the size of the congestion
extern vector<int> state;
extern vector<int> congestion_window_size;
extern vector<int> ssthresh;
extern vector<int> duplicate_ack_count;
extern vector<float> congestion_avoidance_augment;
extern vector<mutex> mtx_state;
extern vector<mutex> mtx_congestion_window_size;
extern vector<mutex> mtx_ssthresh;
extern vector<mutex> mtx_duplicate_ack_count;

/// To handle times
extern vector<mutex> mtx_timeouts;
extern vector<unsigned long> timeouts;
extern vector<double> last_srtt;
extern vector<double> last_sdev;

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

};

void run_receiver(Link *link);

//void run_timeoutter();

void on_new_ack(int proc_num);

void on_duplicate_ack(int proc_num);

void enqueue_new_messages(int proc_num, int number_of_messages);

void on_timeout(int proc_num);

int get_state(int proc_num);

int get_congestion_window_size(int proc_num);

int get_ssthresh(int proc_num);

int get_duplicate_ack_count(int proc_num);

unsigned long get_timeout(int proc_num);

void set_state(int proc_num, int new_value);

int set_congestion_window_size(int proc_num, int new_value);

int increase_congestion_window_size(int proc_num, int st);

void set_ssthresh(int proc_num, int new_value);

void reset_duplicate_ack_count(int proc_num);

void increase_duplicate_ack_count(int proc_num);

void set_timeout(int proc_num, unsigned long new_value);

unsigned long time_milli();

void update_times(int proc_number, unsigned long sending_time);

#endif //DISTRIBUTEDALGORITHMS2019_MANAGER_H
