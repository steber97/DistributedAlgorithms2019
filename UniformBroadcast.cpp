#include "UniformBroadcast.h"
#include "utilities.h"


UniformBroadcast::UniformBroadcast(Link *link, vector<int> &processes, int number_of_messages) {
    this->link = link;
    this->processes = processes;
    this->number_of_messages = number_of_messages;
    this->forward = new unordered_set<message, message_hash, message_equal>;
    this->delivered = new unordered_set<message, message_hash, message_equal>;
    this->acks = new vector<unordered_set<message, message_hash, message_equal>>(number_of_messages);
    this->forward_locked = false;
    this->delivered_locked = false;
    this->acks_locked = false;
}


void UniformBroadcast::init() {
    thread delivery_checker(can_deliver, this);
    delivery_checker.detach();
}


void UniformBroadcast::beb_broadcast(message &msg) {
    for (int process: processes) {
        if (process != link->get_process_number())
            link->send_to(process, msg);
    }
}


void UniformBroadcast::ur_broadcast(message &msg) {
    forward->insert(msg);
    beb_broadcast(msg);
}


void UniformBroadcast::beb_deliver(message &msg) {
    (*acks)[msg.seq_number].insert(msg);
    if (forward->find(msg) == forward->end()) {
        forward->insert(msg);
        beb_broadcast(msg);
    }
}


void UniformBroadcast::ur_deliver(message &msg) {
    //TODO
}


unordered_set<struct message, struct message_hash, struct message_equal> * UniformBroadcast::forwarded_messages() {
    unique_lock<mutex> lck(mtx_forward);
    cv_forward.wait(lck, this->is_forward_locked());
    this->forward_locked = true;
    unordered_set<struct message, struct message_hash, struct message_equal> *forwarded_messages = forward;
    this->forward_locked = false;
    cv_forward.notify_all();
    return forwarded_messages;
}


bool UniformBroadcast::is_delivered(message &msg) {
    unique_lock<mutex> lck(mtx_delivered);
    cv_delivered.wait(lck, this->is_delivered_locked());
    this->delivered_locked = true;
    bool is_delivered = (delivered->find(msg) != delivered->end());
    this->delivered_locked = false;
    cv_delivered.notify_all();
    return is_delivered;
}


int UniformBroadcast::acks_received(message &msg) {
    unique_lock<mutex> lck(mtx_acks);
    cv_acks.wait(lck, this->is_acks_locked());
    this->acks_locked = true;
    int n_acks = (*acks)[msg.seq_number].size();
    this->acks_locked = false;
    cv_acks.notify_all();
    return n_acks;
}


bool UniformBroadcast::is_forward_locked() {
    return forward_locked;
}


bool UniformBroadcast::is_delivered_locked() {
    return delivered_locked;
}


bool UniformBroadcast::is_acks_locked() {
    return acks_locked;
}


void UniformBroadcast::addDelivered(message &msg){
    unique_lock<mutex> lck(mtx_delivered);
    cv_delivered.wait(lck, this->is_delivered_locked());
    this->delivered_locked = true;
    delivered->insert(msg);
    this->delivered_locked = false;
    cv_delivered.notify_all();
}


int UniformBroadcast::get_number_of_messages(){
    return number_of_messages;
}


bool can_deliver(UniformBroadcast* urb) {
    unordered_set<message> forwarded_messages = urb->forwarded_messages();
    for(message m: forwarded_messages){
        if(!urb->is_delivered(m) && (urb->acks_received(m) > urb->get_number_of_messages())){
            urb->addDelivered(m);
            urb->ur_deliver(m);
        }
    }
}



