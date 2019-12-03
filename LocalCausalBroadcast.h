#ifndef DISTRIBUTED_ALGORITHMS_LOCALCAUSALBROADCAST_H
#define DISTRIBUTED_ALGORITHMS_LOCALCAUSALBROADCAST_H

#include "FifoBroadcast.h"
#include "UrBroadcast.h"
#include "utilities.h"


struct LCOBHasher{
    size_t
    operator()(const lcob_message & obj) const
    {
        return obj.first_sender * 10000 + obj.seq_number;
    }
};

struct LCOBComparator
{
    bool
    operator()(const lcob_message & obj1, const lcob_message & obj2) const
    {
        if ((obj1.seq_number == obj2.seq_number) && (obj1.first_sender == obj2.first_sender))
            return true;
        return false;
    }
};

class LocalCausalBroadcast {

private:
    int number_of_processes, number_of_messages;
    FifoBroadcast* fb;
    UrBroadcast* urb;
    bool use_fifo = false;




public:
    vector<int> local_vc;
    unordered_set<lcob_message, LCOBHasher, LCOBComparator> pending;
    vector<vector<int>>* dependencies;
    int process_number;
    LocalCausalBroadcast(int number_of_processes, int number_of_messages, FifoBroadcast* fb, UrBroadcast* urb, vector<vector<int>>* dependencies, const int process_number);
    void lcob_broadcast(lcob_message& lcob_msg);
    void lcob_deliver(lcob_message &msg_to_deliver);
    lcob_message get_next_fifo_delivered();
    lcob_message get_next_urb_delivered();    // This is used to build lcob on top of urb
    void init();  // init method to spawn the thread that will handle the fifo delivery

};

/**
 * This is used to build lcob on top of fifo
 * @param lcob
 */
void handle_fifo_delivered(LocalCausalBroadcast *lcob);

/**
 * This is used to build lcob on top of urb
 * @param lcob
 */
void handle_urb_delivered_lcob(LocalCausalBroadcast *lcob);

#endif //DISTRIBUTED_ALGORITHMS_LOCALCAUSALBROADCAST_H
