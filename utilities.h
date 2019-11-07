
#ifndef PROJECT_TEMPLATE_UTILITIES_H
#define PROJECT_TEMPLATE_UTILITIES_H

#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <mutex>

using namespace std;



extern mutex mtx_log;
extern vector<string> log_actions;



/**
 * This is the broadcast message, it is used at the broadcast level
 * The sender is not the actual sender, but the initial sender of that message.
 *
 * So, if process 3 broadcasts a message received by process 2, the sender is still 2,
 * although it is 3 which is broadcasting. At the message level, instead, the
 * sender is going to be 3.
 */
struct broadcast_message  {

    int seq_number;  // sequence number of the message

    // for example, we may have process 1 broadcasting message 3 received by process 2.
    // In this case, even if the message is broadcasted by 1, the sender is going to be 2.
    int sender;    // initial sender of the message.

    broadcast_message(int seq_number, int sender) {
        // Constructor.
        this->seq_number = seq_number;
        this->sender = sender;
    }
};


/**
 * This is the message structure.
 * ########################
 * ## VERY CAREFUL HERE! ##
 * ########################
 *
 * proc_number is always the sender of the message, not the receiver!!
 * the sequence number of the broadcast message can't be used as a unique identifier for the perfect link message.
 * The way to check for acks is using a vector of sets, vector<unordered_set<tuple<int, int, int>>>
 * The integers are: the process number of the sender of the perfect link message, the original sender of the broadcast
 * message and the sequence number of the original message.
 */
struct message  {
    bool ack;
    int proc_number;
    long long seq_number;   // This sequence number has nothing to do with the sequence number of the broadcast level!
    broadcast_message payload;

    message(bool ack, int proc_number, broadcast_message &payload): payload(payload) {
        // Constructor.
        this->ack = ack;
        this->proc_number = proc_number;
        this->payload = payload;
    }

    // Overloading of constructor with the sequence number.
    message(bool ack, int proc_number, long long seq_n, broadcast_message &payload): payload(payload) {
        // Constructor.
        this->ack = ack;
        this->proc_number = proc_number;
        this->payload = payload;
        this->seq_number = seq_n;
    }

};



/**
 * @param msg is a string formatted like A-ID-SN where:
 *        - A represents the ack
 *        - ID is the process number of sender
 *        - SN is the sequence number of the message
 *        All information must be separated with the character '-'.
 *        An example of message is 1-10-42 which means ack for message 42, sent by process 10
 * @return a struct of type message
 */
message parse_message(string msg);

pair<int, unordered_map<int, pair<string, int>>*> parse_input_data(string &membership_file);

string to_string(message msg);

void broadcast_log(broadcast_message& m);
void delivery_log(broadcast_message& m);

#endif //PROJECT_TEMPLATE_UTILITIES_H

