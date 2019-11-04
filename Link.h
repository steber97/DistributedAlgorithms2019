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

void run_sender(string msg, string ip_address, int port, int destination_process, int sequence_number);
//void run_sender();

class Link {

public:
    Link(int process_number, unordered_map<int, pair<string, int>> *socket_by_process_id);
    void init();
    string get_next_message();
    void send_to(int d_process_number, message& msg, int seq_number);
    unordered_map<int, pair<string, int>> socket_by_process_id;
    int process_number;
};

void run_receiver(string ip_address, int port, Link* link);

#endif //DISTRIBUTEDALGORITHMS2019_MANAGER_H
