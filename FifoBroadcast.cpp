#include "FifoBroadcast.h"

condition_variable cv_fb_delivering_queue;
mutex mtx_fb_delivering_queue;
bool fb_delivering_queue_locked = false;

mutex mtx_pending;

FifoBroadcast::FifoBroadcast(UrBroadcast *urb, int number_of_processes) {
    this->urb = urb;
    this->next_to_deliver.resize(number_of_processes+1, 1);   // they are 1 plus the normal size, as we start counting by 1
    this->pending.resize(number_of_processes+1, queue<int>());
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
        fb->pending[msg.first_sender].push(msg.seq_number);
        bool stop = false;
        while (!stop) {
            stop = true;
            int pending_size = fb->pending[msg.first_sender].size();
            for (int i = 0; i < pending_size; i++) {
                int seq_number = fb->pending[msg.first_sender].front();
                // process number, sequence number.
                fb->pending[msg.first_sender].pop();
                if (seq_number == fb->next_to_deliver[msg.first_sender]) {
                    fb->next_to_deliver[msg.first_sender] ++;  // we have delivered another message.
                    b_message msg_to_deliver(seq_number, msg.first_sender);
                    fb->fb_deliver(msg_to_deliver);
                    stop = false;
                } else
                    fb->pending[msg.first_sender].push(seq_number);
            }
        }
    }
}