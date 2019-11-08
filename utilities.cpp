#include <cassert>
#include <assert.h>
#include "utilities.h"

// these are used to cover the logging functions
mutex mtx_log;
vector<string> log_actions;

// This is the mutex used to communicate with the shared queue between the urb and the beb.
mutex mtx_beb_urb;
queue<urb_message> queue_beb_urb;
bool queue_beb_urb_locked = false;
condition_variable cv_beb_urb;

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
    return {number_of_messages, socket_by_process_id};
}


/**
 * The message format sent at the perfect link layer is :
 * perfect_link/broadcast
 * perfect_link: ack-process_num
 * broadcast: original_sender-seq_number
 *
 * for instance, the message
 * 0-3-1/5-3  means a normal message (ack = 1) sent by process 3 with sequence number pp2p 1,
 * which brings the message 3 originally sent by 5.
 * @param str
 * @return
 */
pp2p_message parse_message(string str) {
    size_t current, previous = 0;
    vector<string> cont_outer;
    char delim_outer = '/';
    current = str.find(delim_outer);
    while (current != string::npos) {
        cont_outer.push_back(str.substr(previous, current - previous));
        previous = current + 1;
        current = str.find(delim_outer, previous);
    }
    cont_outer.push_back(str.substr(previous, current - previous));

    // We need to have a message with size 2 (is like making the split by '/' in python).
    assert(cont_outer.size() == 2);

    // Parse the perfect link message
    previous = 0;
    vector<string> cont1;
    char delim = '-';
    current = cont_outer[0].find(delim);
    while (current != string::npos) {
        cont1.push_back(cont_outer[0].substr(previous, current - previous));
        previous = current + 1;
        current = cont_outer[0].find(delim, previous);
    }
    cont1.push_back(cont_outer[0].substr(previous, current - previous));

    bool ack = stoi(cont1[0]);
    int proc_number = stoi(cont1[1]);
    long long seq_number_pp2p = stoll(cont1[2]);

    // parse the broadcast message.
    previous = 0;
    vector<string> cont2;
    delim = '-';
    current = cont_outer[1].find(delim);
    while (current != string::npos) {
        cont2.push_back(cont_outer[1].substr(previous, current - previous));
        previous = current + 1;
        current = cont_outer[1].find(delim, previous);
    }
    cont2.push_back(cont_outer[1].substr(previous, current - previous));

    int sender = stoi(cont2[0]);
    int seq_number_broad = stoi(cont2[1]);

    // todo parse the rcob_message
    urb_message urb_msg (seq_number_broad, sender);
    pp2p_message pp2p_msg(ack, proc_number, urb_msg);

    return pp2p_msg;
}


/**
 * returns a string of the format:
 * 0-1-3/5-6
 * the first part (before /) is the perfect link message.
 * the second part is the broadcast part.
 *
 * ack - process - seq_number (its long long) / original_sender - sequence_number  (without whitespace)
 * @param msg
 * @return
 */
string to_string(pp2p_message msg){
    return (msg.ack ? string("1") : string("0")) + "-" + to_string(msg.proc_number) + "-" + to_string(msg.seq_number)
                + "/" + to_string(msg.payload.first_sender)
                + "-" + to_string(msg.payload.seq_number);
}


/**
 * Appends the broadcast log to the list of activities.
 * @param m the broadcast message to log
 */
void broadcast_log(urb_message& m) {
    string log_msg = "b " + to_string(m.seq_number) ;
    mtx_log.lock();
    // Append the broadcast log message
    log_actions.push_back(log_msg);
    mtx_log.unlock();
}


/**
 * Appends the broadcast delivery to the list of activities.
 * @param m
 * @param sender
 * @return
 */
void delivery_log(urb_message& m) {
    string log_msg = "d " + to_string(m.first_sender) + " " + to_string(m.seq_number);
    mtx_log.lock();
    log_actions.push_back(log_msg);
    mtx_log.unlock();
}
