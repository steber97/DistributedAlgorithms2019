
#include "LocalCausalBroadcast.h"


LocalCausalBroadcast::LocalCausalBroadcast(int number_of_processes, int number_of_messages, FifoBroadcast *fb) {
    this->number_of_processes = number_of_processes;
    this->number_of_messages = number_of_messages;

    this->fb = fb;
}

void LocalCausalBroadcast::init() {
    thread t_fifo_delivered_handler(handle_fifo_delivered, this);
    t_fifo_delivered_handler.detach();
}

void LocalCausalBroadcast::lcob_broadcast(lcob_message &lcob_msg) {
    // In the meantime just fifo broadcast it!
    b_message bMessage(lcob_msg.seq_number, lcob_msg.first_sender, lcob_msg);
    lcob_broadcast_log(lcob_msg);
    this->fb->fb_broadcast(bMessage);
}

void LocalCausalBroadcast::lcob_deliver(lcob_message &msg_to_deliver) {
    lcob_delivery_log(msg_to_deliver);
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