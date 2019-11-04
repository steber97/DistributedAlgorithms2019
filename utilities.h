
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

bool is_ack(string msg);
void ack_received(string msg);
int unique_id(message &msg);

#endif //PROJECT_TEMPLATE_UTILITIES_H

