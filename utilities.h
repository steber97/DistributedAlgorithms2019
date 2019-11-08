
#ifndef PROJECT_TEMPLATE_UTILITIES_H
#define PROJECT_TEMPLATE_UTILITIES_H

#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <mutex>
#include <condition_variable>
#include <queue>

using namespace std;

extern mutex mtx_log;
extern vector<string> log_actions;


/**
 * This is the message used at FIFO broadcast level
 * It contains a vector of clocks and the actual content of the message
 */
struct rcob_message {
    vector<int> clocks;
    string payload;

    rcob_message(){
        this->clocks = *(new vector<int>);
        this->payload = "";
    }

    rcob_message(vector<int> &clocks, string &payload) {
        // Constructor.
        this->clocks = clocks;
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
struct urb_message  {

    int seq_number;  // sequence number of the message

    // for example, we may have process 1 broadcasting message 3 received by process 2.
    // In this case, even if the message is broadcasted by 1, the sender is going to be 2.
    int first_sender;    // initial sender of the message.
    rcob_message payload;

    urb_message(){
        this->seq_number = 0;
        this->first_sender = 0;
        this->payload = *(new rcob_message());
    }

    urb_message(int seq_number, int first_sender) {
        // Constructor.
        this->seq_number = seq_number;
        this->first_sender = first_sender;
    }

    urb_message(int seq_number, int first_sender, rcob_message &payload) {
        // Constructor.
        this->seq_number = seq_number;
        this->first_sender = first_sender;
        this->payload = payload;
    }
};


/**
 * This is the message structure.
 * ########################
 * ## VERY CAREFUL HERE! ##
 * ########################
 *
 * proc_number is always the sender of the message, not the receiver!!
 *
 */
struct pp2p_message  {
    bool ack;
    int proc_number;
    long long seq_number;   // This sequence number has nothing to do with the sequence number of the broadcast level!
    urb_message payload;

    pp2p_message(){
        this->ack = false;
        this->seq_number = 0;
        this->proc_number = 0;
        this->payload = *(new urb_message());
    }

    pp2p_message(bool ack, int proc_number, urb_message &payload): payload(payload) {
        // Constructor.
        this->ack = ack;
        this->proc_number = proc_number;
        this->payload = payload;
    }

    pp2p_message(bool ack, long long seq_number, int proc_number, urb_message &payload){
        // Constructor.
        this->ack = ack;
        this->seq_number = seq_number;
        this->proc_number = proc_number;
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
pp2p_message parse_message(string msg);

pair<int, unordered_map<int, pair<string, int>>*> parse_input_data(string &membership_file);

string to_string(pp2p_message msg);

void urb_broadcast_log(urb_message& m);
void urb_delivery_log(urb_message& m);


extern condition_variable cv_beb_urb;
extern queue<urb_message> queue_beb_urb;
extern mutex mtx_beb_urb;
extern bool queue_beb_urb_locked;

#endif //PROJECT_TEMPLATE_UTILITIES_H

