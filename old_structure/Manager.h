#ifndef PROJECT_TEMPLATE_MANAGER_H
#define PROJECT_TEMPLATE_MANAGER_H

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
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


using namespace std;

// This is set to true at the beginning of the program.
// As soon as we receive the signal to start the program, then we can set it to false
// and all processes can start sending proper messages instead of initialization messages.
// the access to this variable is protected by a mutex

extern mutex mtx_initialization_phase;
extern bool initialization_phase;

class Link {
    vector<int> ports;
    vector<char*> ips;
    vector<int> processes;
    vector<int> sockfd;
    int process_number;
    int number_of_messages;

public:
    Link(vector<int> &ports, vector<char *> &ips, vector<int> &processes, int process_number,
         int number_of_messages);        // Constructor

    unordered_map<int,int> port_id;

    void run();
};


#endif //PROJECT_TEMPLATE_MANAGER_H
