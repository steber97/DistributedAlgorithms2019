//
// Created by orazio on 11/3/19.
//

#ifndef DISTRIBUTED_ALGORITHMS_UNIFORMBROADCAST_H
#define DISTRIBUTED_ALGORITHMS_UNIFORMBROADCAST_H

#include <unordered_map>
#include <unordered_set>
#include "Link.h"
#include "utilities.h"

using namespace std;

class UniformBroadcast {
private:
    Link *link;
    vector<int> processes;
    int number_of_messages;
    unordered_set<struct message, struct message_hash, struct message_equal> *delivered;
    unordered_set<struct message, struct message_hash, struct message_equal> *forward;
    vector<unordered_set<struct message, struct message_hash, struct message_equal>> *acks;
    condition_variable cv_forward, cv_delivered, cv_acks;
    mutex mtx_forward, mtx_delivered, mtx_acks;
    bool forward_locked, delivered_locked, acks_locked;

public:
    UniformBroadcast(Link *link, vector<int> &processes, int number_of_messages);
    void init();
    void beb_broadcast(message &msg);
    void ur_broadcast(message &msg);
    void beb_deliver(message &msg);
    void ur_deliver(message &msg);
    unordered_set<struct message, struct message_hash, struct message_equal> * forwarded_messages();
    bool is_delivered(message &msg);
    int acks_received(message &msg);
    bool is_forward_locked();
    bool is_delivered_locked();
    bool is_acks_locked();
    int get_number_of_messages();
    void addDelivered(message &msg);
};

bool can_deliver(UniformBroadcast* urb);

#endif //DISTRIBUTED_ALGORITHMS_UNIFORMBROADCAST_H
