#include "RcoBroadcast.h"

condition_variable cv_rcob_delivering_queue;
mutex mtx_rcob_delivering_queue;
bool rcob_delivering_queue_locked = false;

mutex mtx_pending;
condition_variable cv_pending;
bool pending_locked = false;

mutex mtx_clocks;

RcoBroadcast::RcoBroadcast(UrBroadcast *urb, int process_number, int number_of_processes) {
    this->urb = urb;
    this->process_number = process_number;
    this->number_or_processes = number_of_processes;
    this->clocks = new vector<int>(number_of_processes, 0);
    this->pending = new queue<pair<int, rcob_message>>;
}


void RcoBroadcast::init() {
    thread t_urb_delivered_handler(handle_urb_delivered, this);
    thread t_deliver_pending(deliver_pending, this);
    t_urb_delivered_handler.detach();
    t_deliver_pending.detach();
}


void RcoBroadcast::rcob_broadcast(rcob_message &msg) {
    pair<int, rcob_message> msg_to_deliver(process_number, msg);
    //TODO pensare bene su questa cosa
    rcob_deliver(msg_to_deliver);
    urb_message urb_msg(get_clock(process_number), process_number, msg);
    urb->urb_broadcast(urb_msg);
    increase_clock(process_number);
}


void RcoBroadcast::rcob_deliver(pair<int, rcob_message> &msg_to_deliver) {
    unique_lock<mutex> lck(mtx_rcob_delivering_queue);
    cv_rcob_delivering_queue.wait(lck, [&] { return !rcob_delivering_queue_locked; });
    rcob_delivering_queue_locked = true;
    this->rcob_delivering_queue->push(msg_to_deliver);
    rcob_delivering_queue_locked = false;
    cv_rcob_delivering_queue.notify_all();
}


int RcoBroadcast::get_clock(int process) {
    mtx_clocks.lock();
    int process_clock = (*clocks)[process];
    mtx_clocks.unlock();
    return process_clock;
}


void RcoBroadcast::increase_clock(int process) {
    mtx_clocks.lock();
    (*clocks)[process]++;
    mtx_clocks.unlock();
}


pair<int, rcob_message> RcoBroadcast::front_pending() {
    unique_lock<mutex> lck(mtx_pending);
    cv_pending.wait(lck, [&] { return !pending->empty(); });
    pending_locked = true;
    pair<int, rcob_message> front_msg = pending->front();
    pending_locked = false;
    cv_pending.notify_all();
    return front_msg;
}


void RcoBroadcast::pop_pending() {
    unique_lock<mutex> lck(mtx_pending);
    cv_pending.wait(lck, [&] { return !pending->empty(); });
    pending_locked = true;
    pending->pop();
    pending_locked = false;
    cv_pending.notify_all();
}


void RcoBroadcast::push_pending(int first_sender, rcob_message &msg) {
    pair<int, rcob_message> queue_elem(first_sender, msg);
    unique_lock<mutex> lck(mtx_pending);
    cv_pending.wait(lck, [&] { return !pending_locked; });
    pending_locked = true;
    pending->push(queue_elem);
    pending_locked = false;
    cv_pending.notify_all();
}


urb_message RcoBroadcast::get_next_urb_delivered() {
    return urb->get_next_message();
}


int RcoBroadcast::get_process_number() {
    return process_number;
}

int RcoBroadcast::get_number_or_processes() {
    return number_or_processes;
}


void deliver_pending(RcoBroadcast *rcob) {
    while (true) {
        pair<int, rcob_message> inc_msg = rcob->front_pending();
        for (int p = 1; p <= rcob->get_number_or_processes(); p++) {
            if (rcob->get_clock(p) >= inc_msg.second.clocks[p]) {
                rcob->pop_pending();
                rcob->rcob_deliver(inc_msg);
                rcob->increase_clock(p);
            }
        }
    }
}


void handle_urb_delivered(RcoBroadcast *rcob) {
    urb_message urb_delivered_message = rcob->get_next_urb_delivered();
    if (urb_delivered_message.first_sender != rcob->get_process_number()) {
        rcob_message rcob_msg(urb_delivered_message.payload);
        rcob->push_pending(urb_delivered_message.first_sender, rcob_msg);
    }
}
