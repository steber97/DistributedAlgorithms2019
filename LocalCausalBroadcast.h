#ifndef DISTRIBUTED_ALGORITHMS_LOCALCAUSALBROADCAST_H
#define DISTRIBUTED_ALGORITHMS_LOCALCAUSALBROADCAST_H

#include "FifoBroadcast.h"
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

    vector<int> local_vc;
    unordered_set<lcob_message, LCOBHasher, LCOBComparator> pending;
    vector<vector<int>>* dependencies;
    int process_number;


public:
    LocalCausalBroadcast(int number_of_processes, int number_of_messages, FifoBroadcast* fb, vector<vector<int>>* dependencies, const int process_number);
    void lcob_broadcast(lcob_message& lcob_msg);
    void lcob_deliver(lcob_message &msg_to_deliver);
    lcob_message get_next_fifo_delivered();
    void init();  // init method to spawn the thread that will handle the fifo delivery

};

void handle_fifo_delivered(LocalCausalBroadcast *lcob);

#endif //DISTRIBUTED_ALGORITHMS_LOCALCAUSALBROADCAST_H
