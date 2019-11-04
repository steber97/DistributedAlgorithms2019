#ifndef DISTRIBUTED_ALGORITHMS_UNIFORMBROADCAST_H
#define DISTRIBUTED_ALGORITHMS_UNIFORMBROADCAST_H

#include <unordered_set>

#include "Link.h"
#include "utilities.h"

using namespace std;

class UniformBroadcast {
private:
    Link *link;
    vector<int> processes;
    int number_of_messages;
    unordered_set<int> *delivered;
    unordered_set<int> *forward;
    vector<unordered_set<int >> *acks; //a vector (accessible by seq_number of messages) that contains,
                                       // for each message m, a set of the process_numbers of the processes
                                       // who received and then broadcasted successfully m
    condition_variable cv_forward, cv_delivered, cv_acks;
    mutex mtx_forward, mtx_delivered, mtx_acks;
    bool forward_locked, delivered_locked, acks_locked;

public:
    UniformBroadcast(Link *link, vector<int> &processes, int number_of_messages);
    void init();
    void beb_broadcast(message &msg);
    void ur_broadcast(message &msg);
    void beb_deliver(message &msg);
    void ur_deliver(int m_seq_number);
    unordered_set<int> * forwarded_messages();
    bool is_delivered(int m_seq_number);
    int acks_received(int m_seq_number);
    int get_number_of_messages();
    void addDelivered(int m_seq_number);
};

void handle_delivery(UniformBroadcast* urb);

#endif //DISTRIBUTED_ALGORITHMS_UNIFORMBROADCAST_H
