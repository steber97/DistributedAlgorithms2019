
#ifndef PROJECT_TEMPLATE_UTILITIES_H
#define PROJECT_TEMPLATE_UTILITIES_H

#include <iostream>
#include <fstream>
#include <vector>
#include "Link.h"


/**
 *  This is the message structure.
 */
struct message{
    bool ack;
    int seq_number;
    int proc_number;
    string payload;
};

struct message_equal {
public:
    bool operator()(const message& lm, const message& rm) {
        return (lm.ack == rm.ack) && (lm.seq_number == rm.seq_number)
               && (lm.proc_number == rm.proc_number) && (lm.payload == rm.payload);
    }
};

struct message_hash {
public:
    size_t operator()(const message& msg) const {
        return std::hash<int>()((msg.proc_number-1)*1000 + msg.seq_number);
    }
};

unordered_map<int, pair<string, int>>* parse_input_data(string &membership_file);
bool is_ack(string msg);
void ack_received(string msg);
int unique_id(message &msg);

#endif //PROJECT_TEMPLATE_UTILITIES_H

