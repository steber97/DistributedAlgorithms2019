
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

bool check_concurrency_stop(mutex& mtx, bool& variable);


// code freely adapted by https://stackoverflow.com/questions/15160889/how-can-i-make-an-unordered-set-of-pairs-of-integers-in-c
struct pair_hash {
    // only define the hash, as the equal operator is already defined in c++! :)
    inline size_t operator()(const pair<int,int> & v) const {
        return (v.first<<10) + v.second;
    }
};


/**
 * This class is used to perform local causal order broadcast (therefore its name)
 * every message in lcob need to store a vector clock of dependencies.
 * let's say that process i receives a message m from process j:
 * then, the vc of message m stores, for every process, which messages process i must have delivered
 * before being able to deliver m. Of course, process j fills the vc of message m, storing for every process k
 * the last delivered message (by j) of k at the time of broadcasting m. (lol, very confusing, better to plot something to understand)
 * If process k is not a dependency of j, then the vc always stores 0 for k.
 * if process k is j itself (k == j), then the vc stores m-1 (in order to guarantee FIFO).
 *
 */
struct lcob_message {
    int seq_number;
    int first_sender;
    vector<int> vc;  // the vector clock. It must be as long as number_of_processes + 1 (they start from 1, position 0 is always going to be empty).

    /// Initialize lcob message
    lcob_message(int seq_number, int first_sender, vector<int>& vc) {
        this->seq_number = seq_number;
        this->first_sender = first_sender;
        this->vc.resize(vc.size());
        // Copy the vc
        for (size_t i = 0; i<vc.size(); i++ ){
            this->vc[i] = vc[i];
        }
    }

    lcob_message(){
    }
};



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

    lcob_message lcob_m;

    b_message(){
        // Shouldn't be called, but still it needs to be here (I think when initializing the variable when passing parameter to a function)
    }

    b_message(int seq_number, int first_sender, lcob_message& lcob_m){
        // Constructor.
        this->seq_number = seq_number;
        this->first_sender = first_sender;
        this->lcob_m = lcob_m;
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
    b_message payload;

    pp2p_message(bool ack, int proc_number, b_message &payload): payload(payload) {
        // Constructor.
        this->ack = ack;
        this->proc_number = proc_number;
        this->payload = payload;
    }

    pp2p_message(bool ack, long long seq_number, int proc_number, b_message &payload){
        // Constructor.
        this->ack = ack;
        this->seq_number = seq_number;
        this->proc_number = proc_number;
        this->payload = payload;
    }

};

bool is_pp2p_fake(pp2p_message);
pp2p_message create_fake_pp2p(const int number_of_processes);


/**
 * emulates the python split function
 * @param s the string to split
 * @param c the char used to perform the split. must be a char (len==1)!
 * @return the same result of python split method
 */
vector<string>* split(const string& s, char c);


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

pair<unordered_map<int, pair<string, int>> *, vector<vector<int>>*> parse_input_data(string &membership_file);

string to_string(pp2p_message &msg);

/**
 * These functions handle the logging of the output
 */
void urb_broadcast_log(b_message& msg);
void urb_delivery_log(b_message& msg);


void lcob_broadcast_log(lcob_message& msg);
void lcob_delivery_log(lcob_message& msg);


/**
 * These are used to handle the queue that is in the middle between beb and urb,
 * it contains the messages delivered by beb and that should be handled by urb
 */
extern queue<b_message> queue_beb_urb; // the actual queue

extern condition_variable cv_beb_urb;  //
extern mutex mtx_beb_urb;              // to handle concurrency on the queue
extern bool queue_beb_urb_locked;      //

#endif //PROJECT_TEMPLATE_UTILITIES_H

