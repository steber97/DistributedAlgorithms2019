#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <vector>
#include "utilities.h"
#include "Manager.h"


using namespace std;

const char* int_to_char_pointer(int n){
    string temp = to_string(n);
    const char* res = temp.c_str();
    cout << "the number to integer is " << res << " " << strlen(res) << endl;
    return res;
}

Manager* parse_input_data(int argc, char** argv){
    /**
     * Parse command line arguments, and for every process it spawns N different threads:
     * - N-1 to send packets to the N-1 other processes.
     * - 1 to receive all packets
     * - 1 to manage the queue of received messages (shared with the receiver process) and to tell senders
     *      when acks are received (so that stop and wait can be resumed).
     */

    unordered_map<int, pair<string, int>> *socket_by_process_id = new(unordered_map<int, pair<string, int>>);

    if (argc != 3){
        // wrong number of arguments
        cout << "Wrong arguments number!!" << endl;
        exit(EXIT_FAILURE);
    }

    int process_number = atoi(argv[1]);
    string membership_file = argv[2];

    ifstream mem_in(membership_file);

    int number_of_processes ;
    mem_in >> number_of_processes;
    // one of them will remain empty.
    char* addr;
    int process_id;
    string ip;
    int port;
    for (int i = 0; i<number_of_processes; i++){
        mem_in >> process_id;
        mem_in >> ip;
        mem_in >> port;
        (*socket_by_process_id)[process_id] = {ip, port};
    }

    int number_of_messages;
    mem_in >> number_of_messages;

    for (auto el: *socket_by_process_id){
        cout << el.first << " " << el.second.first << " " << el.second.second << endl;
    }

    return new Manager(process_number, *socket_by_process_id);
}