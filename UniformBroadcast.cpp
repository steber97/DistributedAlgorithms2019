#include "UniformBroadcast.h"


UniformBroadcast::UniformBroadcast(Link *link, vector<int> &processes, int number_of_messages) {
    this->link = link;
    this->processes = processes;
    this->number_of_messages = number_of_messages;
    this->forward = new unordered_set<int>;
    this->delivered = new unordered_set<int>;
    this->acks = new vector<unordered_set<int>>(number_of_messages);
    this->forward_locked = false;
    this->delivered_locked = false;
    this->acks_locked = false;
}


void UniformBroadcast::init() {
    thread delivery_checker(handle_delivery, this);
    delivery_checker.detach();
}


void UniformBroadcast::beb_broadcast(message &msg) {
    for (int process: processes) {
        if (process != link->get_process_number())
            link->send_to(process, msg);
    }
}


void UniformBroadcast::ur_broadcast(message &msg) {
    forward->insert(msg.seq_number);
    beb_broadcast(msg);
}


void UniformBroadcast::beb_deliver(message &msg) {
    (*acks)[msg.seq_number].insert(msg.proc_number);
    if (forward->find(msg.seq_number) == forward->end()) {
        forward->insert(msg.seq_number);
        beb_broadcast(msg);
    }
}


void UniformBroadcast::ur_deliver(int m_seq_number) {
    //TODO
}


unordered_set<int > * UniformBroadcast::forwarded_messages() {
    unique_lock<mutex> lck(mtx_forward);
    cv_forward.wait(lck, [&]{ return forward_locked;});
    this->forward_locked = true;
    unordered_set<int > *forwarded_messages = forward;
    this->forward_locked = false;
    cv_forward.notify_all();
    return forwarded_messages;
}


bool UniformBroadcast::is_delivered(int m_seq_number) {
    unique_lock<mutex> lck(mtx_delivered);
    cv_delivered.wait(lck, [&]{ return delivered_locked;});
    this->delivered_locked = true;
    bool is_delivered = (delivered->find(m_seq_number) != delivered->end());
    this->delivered_locked = false;
    cv_delivered.notify_all();
    return is_delivered;
}


int UniformBroadcast::acks_received(int m_seq_number) {
    unique_lock<mutex> lck(mtx_acks);
    cv_acks.wait(lck, [&]{ return acks_locked;});
    this->acks_locked = true;
    int n_acks = (*acks)[m_seq_number].size();
    this->acks_locked = false;
    cv_acks.notify_all();
    return n_acks;
}


void UniformBroadcast::addDelivered(int m_seq_number){
    unique_lock<mutex> lck(mtx_delivered);
    cv_delivered.wait(lck, [&]{return delivered_locked;});
    this->delivered_locked = true;
    delivered->insert(m_seq_number);
    this->delivered_locked = false;
    cv_delivered.notify_all();
}


int UniformBroadcast::get_number_of_messages(){
    return number_of_messages;
}


void handle_delivery(UniformBroadcast* urb) {
    for(int m_seq_number: *urb->forwarded_messages()){
        if(!urb->is_delivered(m_seq_number) && (urb->acks_received(m_seq_number) > urb->get_number_of_messages())){
            urb->addDelivered(m_seq_number);
            urb->ur_deliver(m_seq_number);
        }
    }
}



