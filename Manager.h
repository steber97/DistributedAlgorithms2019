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



using namespace std;

class Manager {
    vector<int> ports;
    vector<char*> ips;
    vector<int> processes;
    int process_number;
    int number_of_messages;
public:
    Manager(vector<int> &ports, vector<char *> &ips, vector<int> &processes, int process_number,
            int number_of_messages);        // Constructor


    void run();
};


#endif //PROJECT_TEMPLATE_MANAGER_H
