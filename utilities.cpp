

#include "utilities.h"

using namespace std;

unordered_map<int, pair<string, int>>* parse_input_data(string &membership_file){
    /**
     * Parse command line arguments, and for every process it spawns N different threads:
     * - N-1 to send packets to the N-1 other processes.
     * - 1 to receive all packets
     * - 1 to manage the queue of received messages (shared with the receiver process) and to tell senders
     *      when acks are received (so that stop and wait can be resumed).
     */

    unordered_map<int, pair<string, int>> *socket_by_process_id = new(unordered_map<int, pair<string, int>>);

    ifstream mem_in(membership_file);

    int number_of_processes ;
    mem_in >> number_of_processes;
    // one of them will remain empty.
    char* addr;
    int pr_n;
    string ip;
    int port;
    for (int i = 0; i<number_of_processes; i++){
        mem_in >> pr_n;
        mem_in >> ip;
        mem_in >> port;
        (*socket_by_process_id)[pr_n] = {ip, port};
    }

    int number_of_messages;
    mem_in >> number_of_messages;

    for (auto el: *socket_by_process_id){
        cout << el.first << " " << el.second.first << " " << el.second.second << endl;
    }

    return socket_by_process_id;
}


bool is_ack(string msg){
    //TODO checks if msg is an ack
}


void ack_received(string msg){
    //TODO sets the correspondig ack to true
}