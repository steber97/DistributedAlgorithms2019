#ifndef DISTRIBUTED_ALGORITHMS_LOCALCAUSALBROADCAST_H
#define DISTRIBUTED_ALGORITHMS_LOCALCAUSALBROADCAST_H

#include "FifoBroadcast.h"
#include "UrBroadcast.h"
#include "utilities.h"

using namespace std;


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

template <typename T>   // T stands for FifoBroadcast or UrBroadcast
class LocalCausalBroadcast {

private:
    int number_of_processes, number_of_messages;

public:
    T* interface;     // can be either fifo or urb
    vector<int> local_vc;
    unordered_set<lcob_message, LCOBHasher, LCOBComparator> pending;
    vector<vector<int>>* dependencies;
    int process_number;
    LocalCausalBroadcast<T>(int number_of_processes, int number_of_messages, T* interface, vector<vector<int>>* dependencies, const int process_number){
        this->number_of_processes = number_of_processes;
        this->number_of_messages = number_of_messages;

        this->local_vc.resize(number_of_processes+1, 0);
        this->dependencies = dependencies;
        this->process_number = process_number;
        this->interface = interface;
    }

    void lcob_broadcast(lcob_message &lcob_msg) {
        // before broadcasting it, you need to update the vc for this message.
        vector<int> vc (this->local_vc.size(), 0);  // copy the vc. set to zero all the entries for which we are not dependent.

        for (size_t i = 0; i < this->dependencies->at(this->process_number).size(); i++){
            // set to values != 0 only the processes we depend on.
            vc[dependencies->at(this->process_number)[i]] = this->local_vc[dependencies->at(this->process_number)[i]];
        }
        vc[dependencies->at(this->process_number)[0]] = lcob_msg.seq_number - 1;   // set the dependency on the previous message of the same sender (FIFO property).
        lcob_msg.vc = vc;
        b_message bMessage(lcob_msg.seq_number, lcob_msg.first_sender, lcob_msg);
        lcob_broadcast_log(lcob_msg);

        this->interface->broadcast(bMessage);  // either with FIFO or URB
    }
    void lcob_deliver(lcob_message &msg_to_deliver) {
        lcob_delivery_log(msg_to_deliver);
    }

    lcob_message get_next_delivered(FifoBroadcast* fifo){
        // this gets the message from the interface.
        // can be either fifo or urb. (depends on the template T).
        return this->interface->get_next_delivered();
    }

    lcob_message get_next_delivered(UrBroadcast* urb){
        // this gets the message from the interface.
        // can be either fifo or urb. (depends on the template T).
        return this->interface->get_next_delivered().lcob_m;
    }

    void init();  // init method, defined below.
};

/**
 * This is used to build lcob on top of fifo
 * @param lcob
 */
template<typename T>
void handle_delivered_lcob(LocalCausalBroadcast<T> *lcob);

template<typename T>
void LocalCausalBroadcast<T>::init() {  // init method to spawn the thread that will handle the fifo delivery
    // This can both use the urb or fifo handler!
    thread t_delivered_handler(handle_delivered_lcob<T>, this);
    t_delivered_handler.detach();

}


/**
 * We can use as template the broadcast we want to use to build on top of it
 * Local Causal Order Broadcast.
 *
 * Can either be URB or FIFO.
 * The important thing is that both FIFO and URB implement a method broadcast
 * (wrapper for fb_broadcast or ur_broadcast).
 * @tparam T
 * @param lcob
 */
template<typename T>
void handle_delivered_lcob(LocalCausalBroadcast<T> *lcob){
    while (true) {
        lcob_message msg = lcob->get_next_delivered(lcob->interface);
        // lcob->lcob_deliver(msg);   // up to now deliver immediately, as if we are doing fifo
        // first check if the message can be delivered immediately (vc is OK)
        bool can_deliver = true;
        for (size_t i = 1; i<lcob->local_vc.size() && can_deliver; i++){
            // we start from 1
            if(lcob->local_vc[i] < msg.vc[i]){
                can_deliver = false;
            }
        }

        if (can_deliver){
            lcob->local_vc[msg.first_sender] ++;
            lcob->lcob_deliver(msg);


            // When you finish dealing with the new message, check for older ones.
            bool at_least_one = true;
            while(at_least_one) {
                at_least_one = false;
                for (lcob_message m: lcob->pending) {
                    bool can_deliver = true;
                    for (size_t i = 1; i < lcob->local_vc.size() && can_deliver; i++) {
                        // we start from 1
                        if (lcob->local_vc[i] < m.vc[i]) {
                            can_deliver = false;
                        }
                    }
                    if (can_deliver) {
                        at_least_one = true;
                        lcob->local_vc[m.first_sender]++;
                        lcob->pending.erase(m);   // remove the message from pending
                        lcob_delivery_log(m);
                    }
                }
            }
        }
        else{
            // we can't do anything more! :/
            lcob->pending.insert(msg);
        }
    }
}


#endif //DISTRIBUTED_ALGORITHMS_LOCALCAUSALBROADCAST_H
