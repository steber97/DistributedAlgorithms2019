
#include "LocalCausalBroadcast.h"


LocalCausalBroadcast::LocalCausalBroadcast(int number_of_processes, int number_of_messages, FifoBroadcast *fb, vector<vector<int>>* dependencies, const int process_number) {
    this->number_of_processes = number_of_processes;
    this->number_of_messages = number_of_messages;

    this->local_vc.resize(number_of_processes+1, 0);

    this->dependencies = dependencies;
    this->process_number = process_number;
    this->fb = fb;
}

void LocalCausalBroadcast::init() {
    thread t_fifo_delivered_handler(handle_fifo_delivered, this);
    t_fifo_delivered_handler.detach();
}

void LocalCausalBroadcast::lcob_broadcast(lcob_message &lcob_msg) {
    // before broadcasting it, you need to update the vc for this message.
    vector<int> vc (this->local_vc.size(), 0);  // copy the vc. set to zero all the entries for which we are not dependent.

    for (int i = 0; i < this->dependencies->at(this->process_number).size(); i++){
        // set to values != 0 only the processes we depend on.
        vc[dependencies->at(this->process_number)[i]] = this->local_vc[dependencies->at(this->process_number)[i]];
    }
    lcob_msg.vc = vc;
    b_message bMessage(lcob_msg.seq_number, lcob_msg.first_sender, lcob_msg);
    lcob_broadcast_log(lcob_msg);
    this->fb->fb_broadcast(bMessage);
}

void LocalCausalBroadcast::lcob_deliver(lcob_message &msg_to_deliver) {
    //lcob_delivery_log(msg_to_deliver);
    // first check if the message can be delivered immediately (vc is OK)
    bool can_deliver = true;
    for (int i = 1; i<this->local_vc.size() && can_deliver; i++){
        // we start from 1
        if(local_vc[i] < msg_to_deliver.vc[i]){
            can_deliver = false;
        }
    }

    if (can_deliver){
        this->local_vc[msg_to_deliver.first_sender] ++;
        lcob_delivery_log(msg_to_deliver);
    }
    else{
        this->pending.insert(msg_to_deliver);
        bool at_least_one = true;
        while(at_least_one) {
            at_least_one = false;
            for (lcob_message m: this->pending) {
                bool can_deliver = true;
                for (int i = 1; i < this->local_vc.size() && can_deliver; i++) {
                    // we start from 1
                    if (local_vc[i] < m.vc[i]) {
                        can_deliver = false;
                    }
                }
                if (can_deliver) {
                    at_least_one = true;
                    this->local_vc[m.first_sender]++;
                    this->pending.erase(m);
                    lcob_delivery_log(m);
                }
            }
        }
    }
}


lcob_message LocalCausalBroadcast::get_next_fifo_delivered() {
    return this->fb->get_next_fifo_delivered();
}


void handle_fifo_delivered(LocalCausalBroadcast *lcob){
    while (true) {
        lcob_message msg = lcob->get_next_fifo_delivered();
        lcob->lcob_deliver(msg);   // up to now deliver immediately, as if we are doing fifo
    }
}