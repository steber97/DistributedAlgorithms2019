//
// Created by stefano on 01/11/19.
//

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

#define MAXLINE 1024

using namespace std;

void send_to(int process_id, string msg);
void run_receiver(string ip_address, int port);

class Manager {
private:
    int process_number;
    unordered_map<int, pair<string, int>> socket_by_process_id;
public:
    Manager(int process_number, unordered_map<int, pair<string, int>> &socket_by_process_id);
    void init();

};


#endif //DISTRIBUTEDALGORITHMS2019_MANAGER_H
