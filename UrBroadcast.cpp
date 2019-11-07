
#include "UrBroadcast.h"


UrBroadcast::UrBroadcast(BeBroadcast *beb, int number_of_processes, int number_of_messages) {
    this->beb = beb;
    this->number_of_processes = number_of_processes;
    this->number_of_messages = number_of_messages;
    this->forward = new unordered_set<int>;
    this->delivered = new unordered_set<int>;
    this->acks = new vector<unordered_set<int>>(number_of_messages);
    this->forward_locked = false;
    this->delivered_locked = false;
    this->acks_locked = false;
}


void UrBroadcast::init() {
    thread delivery_checker(handle_delivery, this);
    delivery_checker.detach();
}


void UrBroadcast::urb_broadcast(broadcast_message &msg) {
    forward->insert(msg.seq_number);
    beb->beb_broadcast(msg);
}

void UrBroadcast::urb_deliver(broadcast_message &msg) {
    (*acks)[msg.seq_number - 1].insert(msg.sender);
    if (forward->find(msg.seq_number) == forward->end()) {
        forward->insert(msg.seq_number);
        beb->beb_broadcast(msg);
    }
}
//
//void Broadcast::beb_deliver(message &msg) {
//    (*acks)[msg.seq_number - 1].insert(msg.proc_number);
//    if (forward->find(msg.seq_number) == forward->end()) {
//        forward->insert(msg.seq_number);
//        beb_broadcast(msg);
//    }
//}


unordered_set<int> *UrBroadcast::forwarded_messages() {
    unique_lock<mutex> lck(mtx_forward);
    cv_forward.wait(lck, [&] { return forward_locked; });
    this->forward_locked = true;
    unordered_set<int> *forwarded_messages = forward;
    this->forward_locked = false;
    cv_forward.notify_all();
    return forwarded_messages;
}


bool UrBroadcast::is_delivered(broadcast_message &msg) {
    unique_lock<mutex> lck(mtx_delivered);
    cv_delivered.wait(lck, [&] { return delivered_locked; });
    this->delivered_locked = true;
    bool is_delivered = (delivered->find(msg.seq_number) != delivered->end());
    this->delivered_locked = false;
    cv_delivered.notify_all();
    return is_delivered;
}


int UrBroadcast::acks_received(broadcast_message &msg) {
    unique_lock<mutex> lck(mtx_acks);
    cv_acks.wait(lck, [&] { return acks_locked; });
    this->acks_locked = true;
    int n_acks = (*acks)[msg.seq_number - 1].size();
    this->acks_locked = false;
    cv_acks.notify_all();
    return n_acks;
}


void UrBroadcast::addDelivered(broadcast_message &msg) {
    unique_lock<mutex> lck(mtx_delivered);
    cv_delivered.wait(lck, [&] { return delivered_locked; });
    this->delivered_locked = true;
    delivered->insert(msg.seq_number);
    this->delivered_locked = false;
    cv_delivered.notify_all();
}


int UrBroadcast::get_number_of_processes() {
    return number_of_processes;
}


void handle_delivery(UrBroadcast *urb) {
    /*
    //TODO lo fa periodicamente
    for (broadcast_message b_msg: *urb->forwarded_messages()) {
        if (!urb->is_delivered(b_msg) && (urb->acks_received(b_msg) > (urb->get_number_of_processes() / 2 + 1))) {
            urb->addDelivered(b_msg);
            urb->urb_deliver(b_msg);
        }
    }
     */
}



