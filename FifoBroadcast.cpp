#include "FifoBroadcast.h"

mutex mtx_pending;

condition_variable cv_fifo_delivering_queue; //
mutex mtx_fifo_delivering_queue;             // To handle concurrency on the queue of the messages delivered at urb level
bool fifo_delivering_queue_locked = false;   //

FifoBroadcast::FifoBroadcast(UrBroadcast *urb, int number_of_processes) {
    this->urb = urb;
    this->next_to_deliver.resize(number_of_processes+1, 1);   // they are 1 plus the normal size, as we start counting by 1
    this->pending.resize(number_of_processes+1, unordered_set<int>());
    this->fifo_delivering_queue = new queue<lcob_message* >;  // Initialize the queue of messages to share with lcobroadcast
}


void FifoBroadcast::init() {
    thread t_urb_delivered_handler(handle_urb_delivered, this);
    t_urb_delivered_handler.detach();
}


void FifoBroadcast::fb_broadcast(b_message msg) {
    urb->urb_broadcast(msg);
}


void FifoBroadcast::fb_deliver(b_message msg_to_deliver) {
    //urb_delivery_log(msg_to_deliver);
    unique_lock<mutex> lck(mtx_fifo_delivering_queue);
    cv_fifo_delivering_queue.wait(lck, [&] { return !fifo_delivering_queue_locked; });
    fifo_delivering_queue_locked = true;

    lcob_message* msg = new lcob_message(msg_to_deliver.seq_number, msg_to_deliver.first_sender, msg_to_deliver.lcob_m.vc);
    this->fifo_delivering_queue->push(msg);
    fifo_delivering_queue_locked = false;
    cv_fifo_delivering_queue.notify_all();
}

b_message FifoBroadcast::get_next_urb_delivered() {
    return urb->get_next_urb_delivered();
}

lcob_message FifoBroadcast::get_next_fifo_delivered() {
    // returns messages to local causal order broadcast
    unique_lock<mutex> lck(mtx_fifo_delivering_queue);
    cv_fifo_delivering_queue.wait(lck, [&] { return !fifo_delivering_queue->empty(); });
    fifo_delivering_queue_locked = true;
    // No idea here why, but I need to use pointers instead of elements...
    lcob_message* next_message = this->fifo_delivering_queue->front();

    this->fifo_delivering_queue->pop();
    fifo_delivering_queue_locked = false;
    cv_fifo_delivering_queue.notify_all();

    // ...and make a copy here
    lcob_message msg (next_message->seq_number, next_message->first_sender, next_message->vc);
    delete(next_message);  // and delete here the pointer in the heap

    // Probably the problem is due to the fact that when passing a copy, it is actually a reference, and every loop in
    // handle_urb_delivered is overridden? (reference to a local variable in the stack.)
    return msg;
}

/**
 * This is just a wrapper, so that both fifo and urb can be used to build on top of it lcob
 * @param msg_to_deliver
 */
void FifoBroadcast::broadcast(b_message msg_to_deliver) {
    // Just invokes fifo broadcast
    this->fb_broadcast(msg_to_deliver);
}

lcob_message FifoBroadcast::get_next_delivered() {
    // this is just a wrapper for get_next_fifo_delivered,
    // so that it can be used by lcob_broadcast.
    return this->get_next_fifo_delivered();
}


void handle_urb_delivered(FifoBroadcast *fb) {
    while (true) {
        b_message msg = fb->get_next_urb_delivered();

        /// Make a very silly assumption:
        // every time you receive a message, you can assume that you can deliver all messages with
        // a lower sequence number starting from the same sender.
        if (fb->next_to_deliver[msg.first_sender] <= msg.seq_number) {
            for (int i = fb->next_to_deliver[msg.first_sender]; i <= msg.seq_number; i++) {
                b_message msg_to_fifo_deliver(i, msg.first_sender, msg.lcob_m);
                fb->fb_deliver(msg_to_fifo_deliver);
            }

            fb->next_to_deliver[msg.first_sender] = msg.seq_number + 1;
        }
    }
}