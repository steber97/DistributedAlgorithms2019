#include "Broadcast.h"


Broadcast::Broadcast(Link *link, int number_of_processes, int number_of_messages) {
    this->link = link;
    this->number_of_processes = number_of_processes;
    this->number_of_messages = number_of_messages;
    this->forward = new unordered_set<int>;
    this->delivered = new unordered_set<int>;
    this->acks = new vector<unordered_set<int>>(number_of_messages);
    this->forward_locked = false;
    this->delivered_locked = false;
    this->acks_locked = false;
}


void Broadcast::init() {
    thread delivery_checker(handle_delivery, this);
    delivery_checker.detach();
}


void Broadcast::beb_broadcast(message &msg) {
    for (int i = 1; i <= number_of_processes + 1; i++) {
        if (number_of_processes != link->get_process_number())
            link->send_to(i, msg);
    }
}


void Broadcast::urb_broadcast(message &msg) {
    forward->insert(msg.seq_number);
    beb_broadcast(msg);
}


void Broadcast::beb_deliver(message &msg) {
    (*acks)[msg.seq_number - 1].insert(msg.proc_number);
    if (forward->find(msg.seq_number) == forward->end()) {
        forward->insert(msg.seq_number);
        beb_broadcast(msg);
    }
}


void Broadcast::urb_deliver(int m_seq_number) {
    //TODO
}


unordered_set<int> *Broadcast::forwarded_messages() {
    unique_lock<mutex> lck(mtx_forward);
    cv_forward.wait(lck, [&] { return forward_locked; });
    this->forward_locked = true;
    unordered_set<int> *forwarded_messages = forward;
    this->forward_locked = false;
    cv_forward.notify_all();
    return forwarded_messages;
}


bool Broadcast::is_delivered(int m_seq_number) {
    unique_lock<mutex> lck(mtx_delivered);
    cv_delivered.wait(lck, [&] { return delivered_locked; });
    this->delivered_locked = true;
    bool is_delivered = (delivered->find(m_seq_number) != delivered->end());
    this->delivered_locked = false;
    cv_delivered.notify_all();
    return is_delivered;
}


int Broadcast::acks_received(int m_seq_number) {
    unique_lock<mutex> lck(mtx_acks);
    cv_acks.wait(lck, [&] { return acks_locked; });
    this->acks_locked = true;
    int n_acks = (*acks)[m_seq_number - 1].size();
    this->acks_locked = false;
    cv_acks.notify_all();
    return n_acks;
}


void Broadcast::addDelivered(int m_seq_number) {
    unique_lock<mutex> lck(mtx_delivered);
    cv_delivered.wait(lck, [&] { return delivered_locked; });
    this->delivered_locked = true;
    delivered->insert(m_seq_number);
    this->delivered_locked = false;
    cv_delivered.notify_all();
}


int Broadcast::get_number_of_messages() {
    return number_of_messages;
}


void handle_delivery(Broadcast *urb) {
    for (int m_seq_number: *urb->forwarded_messages()) {
        if (!urb->is_delivered(m_seq_number) && (urb->acks_received(m_seq_number) > urb->get_number_of_messages())) {
            urb->addDelivered(m_seq_number);
            urb->urb_deliver(m_seq_number);
        }
    }
}



