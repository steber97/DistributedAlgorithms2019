//
// Created by orazio on 11/8/19.
//

#ifndef DISTRIBUTED_ALGORITHMS_FIFOBROADCAST_H
#define DISTRIBUTED_ALGORITHMS_FIFOBROADCAST_H


#include "UrBroadcast.h"

extern atomic<bool> stop_fifo_daemon;

class FifoBroadcast {
private:
    UrBroadcast *urb;

public:
    vector<unordered_set<int>> pending;   // indexed by sender and sequence number;
    vector<int> next_to_deliver;
    FifoBroadcast(UrBroadcast *urb,int number_of_processes);
    void init();
    void fb_broadcast(b_message &fifo_msg);
    void fb_deliver(b_message &msg_to_deliver);
    b_message get_next_urb_delivered();
};

void handle_urb_delivered(FifoBroadcast *fb);

#endif //DISTRIBUTED_ALGORITHMS_FIFOBROADCAST_H
