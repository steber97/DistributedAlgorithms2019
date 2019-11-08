#include "FifoBroadcast.h"

condition_variable cv_fb_delivering_queue;
mutex mtx_fb_delivering_queue;
bool fb_delivering_queue_locked = false;

mutex mtx_pending;

FifoBroadcast::FifoBroadcast(UrBroadcast *urb, int number_of_processes) {
    this->urb = urb;
    this->next_to_deliver.resize(number_of_processes, 1);
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


void FifoBroadcast::push_pending(pair<int, int> &pending_msg) {
    mtx_pending.lock();
    pending.push(pending_msg);
    mtx_pending.unlock();
}


int FifoBroadcast::pending_size() {
    mtx_pending.lock();
    int size = pending.size();
    mtx_pending.unlock();
    return size;
}


pair<int, int> FifoBroadcast::pop_pending() {
    mtx_pending.lock();
    pair<int, int> pending_msg = pending.front();
    pending.pop();
    mtx_pending.unlock();
    return pending_msg;
}

b_message FifoBroadcast::get_next_urb_delivered() {
    return urb->get_next_urb_delivered();
}


void handle_urb_delivered(FifoBroadcast *fb) {
    while (true) {
        b_message msg = fb->get_next_urb_delivered();
        pair<int, int> pend_msg(msg.first_sender, msg.seq_number);
        fb->pending.push(pend_msg);
        bool stop = false;
        while (!stop) {
            stop = true;
            int pending_size = fb->pending.size();
            for (int i = 0; i < pending_size; i++) {
                pair<int, int> pending_msg = fb->pending.front();
                // process number, sequence number.
                fb->pending.pop();
                if (pending_msg.second == fb->next_to_deliver[pending_msg.first-1] ) {
                    fb->next_to_deliver[pending_msg.first-1] ++;
                    b_message msg_to_deliver(pending_msg.second, pending_msg.first);
                    fb->fb_deliver(msg_to_deliver);
                    stop = false;
                } else
                    fb->pending.push(pending_msg);
            }
        }
    }
}