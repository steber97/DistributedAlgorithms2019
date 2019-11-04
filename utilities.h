
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
 */
struct message{
    bool ack;
    int seq_number;
    int proc_number;
    string payload;
};

/**
 * Returns a struct of type message.
 * it contains ack, seq_number and proc_number of the sender
 * @param msg a string formatted like:
 *          ack
 *          process id of sender
 *          sequence number
 *
 *          All information is separated with the character -
 *          an example of message is 1-10-42 which means ack for message 42, sent by process 10
 *          or 0-11-
 * @return
 */
message parse_message(string msg);
pair<int, unordered_map<int, pair<string, int>>*> parse_input_data(string &membership_file);
bool is_ack(string msg);
void ack_received(string msg);



#endif //PROJECT_TEMPLATE_UTILITIES_H

