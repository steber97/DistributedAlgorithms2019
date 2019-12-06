#ifndef DISTRIBUTED_ALGORITHMS_URBROADCAST_H
#define DISTRIBUTED_ALGORITHMS_URBROADCAST_H

#include <unordered_set>
#include <condition_variable>
#include <unordered_map>

#include "BeBroadcast.h"

using namespace std;


class UrBroadcast {
private:
    int number_of_processes;
    int number_of_messages;
    queue<b_message> *urb_delivering_queue;
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
    b_message get_next_delivered();  //  a wrapper for the local_causal_reliable_broadcast.
    void broadcast(b_message &msg);
    bool is_delivered(b_message &msg);
    int acks_received(b_message &msg);
    int get_number_of_processes();
    void addDelivered(b_message &msg);
};

void handle_beb_delivery(UrBroadcast* urb);

#endif //DISTRIBUTED_ALGORITHMS_URBROADCAST_H
