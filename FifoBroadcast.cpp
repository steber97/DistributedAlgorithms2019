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
    urb_broadcast_log(msg);
}


void FifoBroadcast::fb_deliver(b_message &msg_to_deliver) {
    urb_delivery_log(msg_to_deliver);
    /*
    unique_lock<mutex> lck(mtx_fb_delivering_queue);
    cv_fb_delivering_queue.wait(lck, [&] { return !fb_delivering_queue_locked; });
    fb_delivering_queue_locked = true;
    this->fb_delivering_queue->push(msg_to_deliver);
    fb_delivering_queue_locked = false;
    cv_fb_delivering_queue.notify_all();
     */
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


int FifoBroadcast::get_next_to_deliver(int process) {
    mtx_next_to_deliver.lock();
    int next_msg_to_deliver = (*next_to_deliver)[process - 1];
    mtx_next_to_deliver.unlock();
    return next_msg_to_deliver;
}


void FifoBroadcast::increase_next_to_deliver(int process) {
    mtx_next_to_deliver.lock();
    (*next_to_deliver)[process - 1]++;
    mtx_next_to_deliver.unlock();
}


b_message FifoBroadcast::get_next_urb_delivered() {
    return urb->get_next_urb_delivered();
}

b_message FifoBroadcast::get_next_fifo_delivered() {
    unique_lock<mutex> lck(mtx_fb_delivering_queue);
    cv_fb_delivering_queue.wait(lck, [&] { return !fb_delivering_queue->empty(); });
    fb_delivering_queue_locked = true;
    b_message next_message = this->fb_delivering_queue->front();
    this->fb_delivering_queue->pop();
    fb_delivering_queue_locked = false;
    cv_fb_delivering_queue.notify_all();
    return next_message;
}


void handle_urb_delivered(FifoBroadcast *fb) {
    while (!check_concurrency_stop(mtx_fifo, stop_fifo)) {
        b_message msg = fb->get_next_urb_delivered();
        pair<int, int> pend_msg(msg.first_sender, msg.seq_number);
        fb->push_pending(pend_msg);
        /*
        cout << "{";
        for (int j = 0; j < fb->pending_size(); j++) {
            pair<int, int> elem = fb->pop_pending();
            cout << "[" << elem.first << ", " << elem.second << "]\t";
        }
        cout << "}" << endl;
         */
        bool stop = false;
        while (!stop) {
            stop = true;
            int pending_size = fb->pending_size();
            for (int i = 0; i < pending_size; i++) {
                pair<int, int> pending_msg = fb->pop_pending();
                if (pending_msg.second == fb->get_next_to_deliver(pending_msg.first)) {
                    fb->increase_next_to_deliver(pending_msg.first);
                    b_message msg_to_deliver(pending_msg.second, pending_msg.first);
                    fb->fb_deliver(msg_to_deliver);
                    stop = false;
                } else
                    fb->push_pending(pending_msg);
            }
        }
    }
}