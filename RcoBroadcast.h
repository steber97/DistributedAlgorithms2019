#include "UrBroadcast.h"

#ifndef DISTRIBUTED_ALGORITHMS_RCOBROADCAST_H
#define DISTRIBUTED_ALGORITHMS_RCOBROADCAST_H


class RcoBroadcast {
private:
    UrBroadcast *urb;
    int process_number;
    int number_or_processes;
    vector<int> *clocks;
    queue<pair<int, rcob_message>> *pending;
    queue<pair<int, rcob_message>> *rcob_delivering_queue;

public:
    RcoBroadcast(UrBroadcast* urb,  int process_number, int number_of_processes);
    void init();
    void rcob_broadcast(rcob_message &msg);
    void rcob_deliver(pair<int, rcob_message> &msg_to_deliver);
    int get_clock(int process);
    void increase_clock(int process);
    pair<int, rcob_message> front_pending();
    void pop_pending();
    void push_pending(int first_sender, rcob_message &msg);
    urb_message get_next_urb_delivered();
    int get_process_number();
    int get_number_or_processes();
};

void deliver_pending(RcoBroadcast *rcob);
void handle_urb_delivered(RcoBroadcast *rcob);


#endif //DISTRIBUTED_ALGORITHMS_RCOBROADCAST_H
