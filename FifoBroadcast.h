//
// Created by orazio on 11/8/19.
//

#ifndef DISTRIBUTED_ALGORITHMS_FIFOBROADCAST_H
#define DISTRIBUTED_ALGORITHMS_FIFOBROADCAST_H


#include "UrBroadcast.h"

class FifoBroadcast {
private:
    UrBroadcast *urb;
    int local_sequence_number;
    vector<int> *next_to_deliver;
    unordered_set<pair<int, int>, pair_hash> *pending; //set of pending messages stored as pair of int,
                                                       //the first one is the process number of the sender,
                                                       //the second one is the sequence number of the message
    queue<b_message> *fb_delivering_queue;
public:
    FifoBroadcast(UrBroadcast *urb,int number_of_processes);
    void init();
    void fb_broadcast(b_message &fifo_msg);
    void fb_deliver(b_message &msg_to_deliver);
    void add_pending(b_message &msg);
    void remove_pending(int sender, int seq_number);
    unordered_set<pair<int, int>, struct pair_hash> * get_pending_copy();
    int get_next_to_deliver(int process);
    void increase_next_to_deliver(int process);
    b_message get_next_urb_delivered();
    b_message get_next_fifo_delivered();
};

void handle_urb_delivered(FifoBroadcast *fb);

#endif //DISTRIBUTED_ALGORITHMS_FIFOBROADCAST_H
