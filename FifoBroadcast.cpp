#include "FifoBroadcast.h"

mutex mtx_pending;

FifoBroadcast::FifoBroadcast(UrBroadcast *urb, int number_of_processes) {
    this->urb = urb;
    this->next_to_deliver.resize(number_of_processes+1, 1);   // they are 1 plus the normal size, as we start counting by 1
    this->pending.resize(number_of_processes+1, unordered_set<int>());
}


void FifoBroadcast::init() {
    thread t_urb_delivered_handler(handle_urb_delivered, this);
    t_urb_delivered_handler.detach();
}


void FifoBroadcast::fb_broadcast(b_message &msg) {
    urb_broadcast_log(msg);
    urb->urb_broadcast(msg);
}


void FifoBroadcast::fb_deliver(b_message &msg_to_deliver) {
    urb_delivery_log(msg_to_deliver);
}

b_message FifoBroadcast::get_next_urb_delivered() {
    return urb->get_next_urb_delivered();
}


void handle_urb_delivered(FifoBroadcast *fb) {
    while (true) {
        b_message msg = fb->get_next_urb_delivered();

        /// Make a very silly assumption:
        // every time you receive a message, you can assume that you can deliver all messages with
        // a lower sequence number starting from the same sender.
        if (fb->next_to_deliver[msg.first_sender] <= msg.seq_number) {
            for (int i = fb->next_to_deliver[msg.first_sender]; i <= msg.seq_number; i++) {
                b_message msg_to_fifo_deliver(i, msg.first_sender, msg.lcob_m);
                fb->fb_deliver(msg_to_fifo_deliver);
            }

            fb->next_to_deliver[msg.first_sender] = msg.seq_number + 1;
        }
    }
}