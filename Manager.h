//
// Created by stefano on 31/10/19.
//

#ifndef PROJECT_TEMPLATE_MANAGER_H
#define PROJECT_TEMPLATE_MANAGER_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <thread>
#include <vector>



using namespace std;

class Manager {

    vector<int> ports;
    vector<char*> ips;
    vector<int> processes;
    int s_port;
    char*  addr;
    int process_number;
public:
    Manager(vector<int>& ports, vector<char*>& ips, vector<int>& processes, int s_port, char* addr, int process_number);        // Constructor


    void run();
};


#endif //PROJECT_TEMPLATE_MANAGER_H
