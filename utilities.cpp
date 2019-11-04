#include "utilities.h"

/**
 * Parses the input file
 * @param membership_file the file to parse
 * @return a pair of:
 *          - the total number of message to be sent by each process
 *          - a map that contains info on ip and port for each process
 */
pair<int, unordered_map<int, pair<string, int>>*> parse_input_data(string &membership_file){
    unordered_map<int, pair<string, int>> *socket_by_process_id = new(unordered_map<int, pair<string, int>>);
    ifstream mem_in(membership_file);
    int number_of_processes ;
    mem_in >> number_of_processes;
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
    return {number_of_messages, socket_by_process_id};
}


message parse_message(string str) {
    size_t current, previous = 0;
    vector<string> cont;
    char delim = '-';
    current = str.find(delim);
    while (current != string::npos) {
        cont.push_back(str.substr(previous, current - previous));
        previous = current + 1;
        current = str.find(delim, previous);
    }
    cont.push_back(str.substr(previous, current - previous));

    message m;
    m.ack = bool(stoi(cont[0]));
    m.proc_number = stoi(cont[1]);
    m.seq_number = stoi(cont[2]);

    return m;
}


int unique_id(message &msg, int number_of_messages) {
    return (msg.proc_number - 1) * number_of_messages + msg.seq_number;
}




bool is_ack(string msg){
    /**
     * a message is an ack if the first bit is 1
     * if it is a message, the first bit is 0.
     */
    cout << " check if msg is ack " << msg << endl;
    return msg[0] == '1';
}


void ack_received(string msg){
    //TODO sets the correspondig ack to true

}


