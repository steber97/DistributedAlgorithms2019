#ifndef DISTRIBUTED_ALGORITHMS_URBROADCAST_H
#define DISTRIBUTED_ALGORITHMS_URBROADCAST_H

#include <unordered_set>
#include <condition_variable>
#include "BeBroadcast.h"
#include "utilities.h"

using namespace std;

class UrBroadcast {
private:
    BeBroadcast *beb;
    int number_of_processes;
    int number_of_messages;
    unordered_set<int> *delivered;
    unordered_set<int> *forward;
    vector<unordered_set<int >> *acks; //a vector (accessible by seq_number of messages) that contains,
                                       // for each pp2p_message m, a set of the process_numbers of the processes
                                       // from who the current process received m back
    condition_variable cv_forward, cv_delivered, cv_acks;
    mutex mtx_forward, mtx_delivered, mtx_acks;
    bool forward_locked, delivered_locked, acks_locked;
    queue<urb_message> *urb_delivering_queue;
public:
    UrBroadcast(BeBroadcast *beb, int number_of_processes, int number_of_messages);
    void init();
    void urb_broadcast(urb_message &msg);
    void urb_deliver(urb_message &msg);
    urb_message get_next_message();
    unordered_set<int> * forwarded_messages();
    bool is_delivered(urb_message &msg);
    int acks_received(urb_message &msg);
    int get_number_of_processes();
    void addDelivered(urb_message &msg);
};

void handle_delivery(UrBroadcast* urb);

#endif //DISTRIBUTED_ALGORITHMS_URBROADCAST_H
