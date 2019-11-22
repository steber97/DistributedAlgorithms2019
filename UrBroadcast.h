#ifndef DISTRIBUTED_ALGORITHMS_URBROADCAST_H
#define DISTRIBUTED_ALGORITHMS_URBROADCAST_H

#include <unordered_set>
#include <condition_variable>
#include <unordered_map>

#include "readerwriterqueue.h"
#include "BeBroadcast.h"

using namespace moodycamel;

extern atomic<bool> stop_urb_daemon;

// code freely adapted by https://stackoverflow.com/questions/15160889/how-can-i-make-an-unordered-set-of-pairs-of-integers-in-c
struct pair_hash {
    // only define the hash, as the equal operator is already defined in c++! :)
    inline size_t operator()(const pair<int,int> & v) const {
        return (v.first<<10) + v.second;
    }
};


class UrBroadcast {
private:
    int number_of_processes;
    int number_of_messages;
public:
    BeBroadcast *beb;
    unordered_set<pair<int,int>, pair_hash> delivered; // both delivered and forward are indexed by sender, sequence number.
    unordered_set<pair<int,int>, pair_hash> forward;
    unordered_map<pair<int,int>, int, pair_hash> acks; // an unordered map which maps, per each message (sender, seq_number)
    // the number of replicates for this response.
    // When the number exceeds half the processes + 1, then the message can be
    // delivered by urb.
    mutex mtx_forward, mtx_delivered, mtx_acks;

    UrBroadcast(BeBroadcast *beb, int number_of_processes, int number_of_messages);
    void init();
    void urb_broadcast(b_message &msg);
    void urb_deliver(b_message &msg);
    b_message get_next_urb_delivered();
    bool is_delivered(b_message &msg);
    int acks_received(b_message &msg);
    int get_number_of_processes();
    void addDelivered(b_message &msg);
    b_message get_next_beb_delivered();
};

void handle_beb_delivery(UrBroadcast* urb);

#endif //DISTRIBUTED_ALGORITHMS_URBROADCAST_H
