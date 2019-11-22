#include "FifoBroadcast.h"

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

#ifdef DEBUG
    cout << link->get_process_number() << " URB-delivered the message " << msg.seq_number << " sent by " << msg.first_sender << endl;
#endif
}

b_message FifoBroadcast::get_next_urb_delivered() {
    return urb->get_next_urb_delivered();
}


void handle_urb_delivered(FifoBroadcast *fb) {
    while (true) {
        b_message msg = fb->get_next_urb_delivered();
        fb->pending[msg.first_sender].insert(msg.seq_number);
        while(fb->pending[msg.first_sender].find(fb->next_to_deliver[msg.first_sender]) != fb->pending[msg.first_sender].end()){
            int seq_number = fb->next_to_deliver[msg.first_sender];
            fb->next_to_deliver[msg.first_sender] ++;
            b_message msg_to_deliver(seq_number, msg.first_sender);
            fb->fb_deliver(msg_to_deliver);
            fb->pending[msg.first_sender].erase(seq_number);
        }
    }
}