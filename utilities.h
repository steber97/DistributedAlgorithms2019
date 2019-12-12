
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
#include <assert.h>

using namespace std;

extern mutex mtx_log;
extern vector<string> log_actions;

extern mutex mtx_pp2p_receiver, mtx_pp2p_sender, mtx_pp2p_get_msg;
extern bool stop_pp2p_receiver;
extern bool stop_pp2p_sender;
extern bool stop_pp2p_get_msg;


/**
 * This is the broadcast message, it is used at the broadcast level
 * The sender is not the actual sender, but the initial sender of that message.
 *
 * So, if process 3 broadcasts a message received by process 2, the sender is still 2,
 * although it is 3 which is broadcasting.
 */
struct b_message  {

    int seq_number;  // sequence number of the message

    // for example, we may have process 1 broadcasting message 3 received by process 2.
    // In this case, even if the message is broadcasted by 1, the sender is going to be 2.
    int first_sender;    // initial sender of the message.

    b_message(){
        this->seq_number = 0;
        this->first_sender = 0;
    }

    b_message(int seq_number, int first_sender) {
        // Constructor.
        this->seq_number = seq_number;
        this->first_sender = first_sender;
    }
};


/**
 * This is the message structure at link level.
 *
 * proc_number is always the sender of the message, not the receiver! In addition it is the process who actually sent
 * the message, not the one who initially sent it in broadcast.
 *
 */
struct pp2p_message  {
    bool ack;
    int proc_number;
    long long seq_number;   // This sequence number has nothing to do with the sequence number of the broadcast level!
    unsigned long int timestamp;
    b_message payload;

    pp2p_message(bool ack, int proc_number, unsigned long timestamp, b_message &payload): payload(payload) {
        // Constructor.
        this->ack = ack;
        this->proc_number = proc_number;
        this->timestamp = timestamp;
        this->payload = payload;
    }

    pp2p_message(bool ack, long long seq_number, int proc_number, unsigned long timestamp, b_message &payload){
        // Constructor.
        this->ack = ack;
        this->seq_number = seq_number;
        this->proc_number = proc_number;
        this->timestamp = timestamp;
        this->payload = payload;
    }

};

bool is_pp2p_fake(pp2p_message);
pp2p_message create_fake_pp2p();

/**
 * @param msg is a string formatted like A-ID-SN where:
 *        - A represents the ack
 *        - ID is the process number of sender
 *        - SN is the sequence number of the message
 *        All information must be separated with the character '-'.
 *        An example of message is 1-10-42 which means ack for message 42, sent by process 10
 * @return a struct of type message
 */
pp2p_message parse_message(const string &msg);

unordered_map<int, pair<string, int>>*parse_input_data(string &membership_file);

string to_string(pp2p_message &msg);

/**
 * These functions handle the logging of the output
 */
void urb_broadcast_log(b_message& msg);
void urb_delivery_log(b_message& msg);

void log(string s, int proc_num);


/**
 * These are used to handle the queue that is in the middle between beb and urb,
 * it contains the messages delivered by beb and that should be handled by urb
 */
extern queue<b_message> queue_beb_urb; // the actual queue

extern condition_variable cv_beb_urb;  //
extern mutex mtx_beb_urb;              // to handle concurrency on the queue
extern bool queue_beb_urb_locked;      //

#endif //PROJECT_TEMPLATE_UTILITIES_H

