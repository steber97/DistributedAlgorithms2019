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

    cout << "parsing input data" << endl;

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
    cout << "The number of processes is " << number_of_processes << endl;
    // one of them will remain empty.
    vector<int> ports(number_of_processes);
    vector<char*> ips(number_of_processes);
    vector<int> processes(number_of_processes);
    int s_port ;
    char* addr;
    for (int i = 0; i<number_of_processes; i++){

        mem_in >> processes[i];
        ips[i] = new char[10];
        mem_in >> ips[i] ;
        mem_in >> ports[i];
        cout << processes[i] << " " << ips[i] << " " << ports[i] << " " << endl;
        if (i+1 == process_number){
            s_port = ports[i];
            addr = ips[i];
        }
    }

    int number_of_messages;
    mem_in >> number_of_messages;
    cout << "number of messages " << number_of_messages << endl;

    Manager* m = new Manager(ports, ips, processes, process_number, number_of_messages);
    return m;
}