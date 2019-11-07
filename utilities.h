
#ifndef PROJECT_TEMPLATE_UTILITIES_H
#define PROJECT_TEMPLATE_UTILITIES_H

#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <unordered_map>

using namespace std;

/**
 * This is the message structure.
 * ########################
 * ## VERY CAREFUL HERE! ##
 * ########################
 *
 * proc_number is always the sender of the message, not the receiver!!
 *
 */
struct message  {
    bool ack;
    int seq_number;
    int proc_number;
    string payload;

    message(){
        this->ack = false;
        this->seq_number = 0;
        this->proc_number = 0;
        this->payload = "";
    }

    message(bool ack, int seq_number, int proc_number, string payload){
        // Constructor.
        this->ack = ack;
        this->seq_number = seq_number;
        this->proc_number = proc_number;
        this->payload = payload;
    }
};


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
    message payload;   // the paylaod is a perfect link message.

    broadcast_message(int seq_number, int sender, message& payload){
        // Constructor.
        this->seq_number = seq_number;
        this->sender = sender;
        this->payload = payload;
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

string create_broadcast_log(message m);

string create_delivery_log(message m, int sender);

#endif //PROJECT_TEMPLATE_UTILITIES_H

