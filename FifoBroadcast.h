//
// Created by orazio on 11/8/19.
//

#ifndef DISTRIBUTED_ALGORITHMS_FIFOBROADCAST_H
#define DISTRIBUTED_ALGORITHMS_FIFOBROADCAST_H


#include "UrBroadcast.h"

class FifoBroadcast {
private:
    UrBroadcast *urb;

public:
    //queue<pair<int, int>> pending; //set of pending messages stored as pair of int,
                                    //the first one is the process number of the sender,
                                    //the second one is the sequence number of the messag

    vector<unordered_set<int>> pending;   // indexed by sender and sequence number;
    vector<int> next_to_deliver;
    FifoBroadcast(UrBroadcast *urb,int number_of_processes);
    void init();
    void fb_broadcast(b_message fifo_msg);
    void fb_deliver(b_message msg_to_deliver);
    void broadcast(b_message);
    b_message get_next_urb_delivered();

    lcob_message get_next_fifo_delivered();
    lcob_message get_next_delivered();
    queue<lcob_message*> *fifo_delivering_queue;
};

void handle_urb_delivered(FifoBroadcast *fb);

#endif //DISTRIBUTED_ALGORITHMS_FIFOBROADCAST_H
