#include "FifoBroadcast.h"

condition_variable cv_fb_delivering_queue;
mutex mtx_fb_delivering_queue;
bool fb_delivering_queue_locked = false;

mutex mtx_pending;

mutex mtx_next_to_deliver;


FifoBroadcast::FifoBroadcast(UrBroadcast *urb, int number_of_processes) {
    this->urb = urb;
    this->next_to_deliver = new vector<int>(number_of_processes, 1);
    this->local_sequence_number = 0;

}


void FifoBroadcast::init() {
    thread t_urb_delivered_handler(handle_urb_delivered, this);
    t_urb_delivered_handler.detach();
}


void FifoBroadcast::fb_broadcast(b_message &msg) {
    local_sequence_number++;
    urb->urb_broadcast(msg);
}


void FifoBroadcast::fb_deliver(b_message &msg_to_deliver) {
    unique_lock<mutex> lck(mtx_fb_delivering_queue);
    cv_fb_delivering_queue.wait(lck, [&] { return !fb_delivering_queue_locked; });
    fb_delivering_queue_locked = true;
    this->fb_delivering_queue->push(msg_to_deliver);
    fb_delivering_queue_locked = false;
    cv_fb_delivering_queue.notify_all();
}


void FifoBroadcast::add_pending(b_message &msg) {
    pair<int, int> new_pending(msg.first_sender, msg.seq_number);
    mtx_pending.lock();
    pending->insert(new_pending);
    mtx_pending.unlock();
}


void FifoBroadcast::remove_pending(int sender, int seq_number) {
    pair<int, int> msg_to_remove(sender, seq_number);
    mtx_pending.lock();
    pending->erase(msg_to_remove);
    mtx_pending.unlock();
}


unordered_set<pair<int, int>, pair_hash>* FifoBroadcast::get_pending_copy() {
    auto* pending_copy = new unordered_set<pair<int, int>, pair_hash>();
    mtx_pending.lock();
    for(auto el : *pending) {
        pair<int, int> pending_msg(el.first, el.second);
        pending_copy->insert(pending_msg);
    }
    mtx_pending.unlock();
    return pending_copy;
}


int FifoBroadcast::get_next_to_deliver(int process) {
    mtx_next_to_deliver.lock();
    int next_msg_to_deliver = (*next_to_deliver)[process];
    mtx_next_to_deliver.unlock();
    return next_msg_to_deliver;
}


void FifoBroadcast::increase_next_to_deliver(int process) {
    mtx_next_to_deliver.lock();
    (*next_to_deliver)[process]++;
    mtx_next_to_deliver.unlock();
}


b_message FifoBroadcast::get_next_urb_delivered() {
    return urb->get_next_message();
}


void handle_urb_delivered(FifoBroadcast *fb) {
    while (true) {
        b_message msg = fb->get_next_urb_delivered();
        fb->add_pending(msg);
        for (pair<int, int> p : *fb->get_pending_copy()) {
            if (p.second == fb->get_next_to_deliver(p.first)) {
                fb->increase_next_to_deliver(p.first);
                fb->remove_pending(p.first, p.second);
                b_message msg_to_deliver(p.second, p.first);
                fb->fb_deliver(msg_to_deliver);
            }
        }
    }
}