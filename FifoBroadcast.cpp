#include "FifoBroadcast.h"

mutex mtx_pending;

condition_variable cv_fifo_delivering_queue; //
mutex mtx_fifo_delivering_queue;             // To handle concurrency on the queue of the messages delivered at urb level
bool fifo_delivering_queue_locked = false;   //

FifoBroadcast::FifoBroadcast(UrBroadcast *urb, int number_of_processes) {
    this->urb = urb;

    // next to deliver stores which is the next message we need to deliver for every process, in order to respect the FIFO property.
    this->next_to_deliver.resize(number_of_processes+1, 1);   // they are 1 plus the normal size, as we start counting by 1
    this->pending.resize(number_of_processes+1, unordered_set<int>());
    this->fifo_delivering_queue = new queue<lcob_message* >;  // Initialize the queue of messages to share with lcobroadcast
}

/**
 * Detaches the thread that will receives messages retrieved from lower layers (URB).
 */
void FifoBroadcast::init() {
    thread t_urb_delivered_handler(handle_urb_delivered, this);
    t_urb_delivered_handler.detach();
}



/**
 * Fifo broadcast a message, is actually the same as urb broadcasting it.
 */
void FifoBroadcast::fb_broadcast(b_message msg) {
    urb->urb_broadcast(msg);
}

/**
 * Delivers a message to the upper layers
 * (messages are pushed into a queue of messages which can be accessed concurrently).
 * @param msg_to_deliver
 */
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


/**
 * Gets next message coming from the lower layer (urb)
 * @return
 */
b_message FifoBroadcast::get_next_urb_delivered() {
    return urb->get_next_urb_delivered();
}


/**
 * method exposed for upper layers to get the next fifo delivered message.
 * Can be blocking, when there is no message to deliver.
 * Uses condition variables and notify in order not to waste resources doing busy waiting.
 * @return
 */
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
 * Just an alias for fb_broadcast, so that it can be used by template class
 */
void FifoBroadcast::broadcast(b_message msg_to_deliver) {
    // Just invokes fifo broadcast
    this->fb_broadcast(msg_to_deliver);
}


/**
 * just an alias for get_next_fifo_delivered()
 * @return
 */
lcob_message FifoBroadcast::get_next_delivered() {
    // this is just a wrapper for get_next_fifo_delivered,
    // so that it can be used by lcob_broadcast.
    return this->get_next_fifo_delivered();
}


/**
 * Decides when it is the case to deliver messages coming from the lower layers.
 * It has to respect FIFO order, so whenever we receive messages from the same
 * process but out of order, we need to wait and deliver first the remaining messages
 * with a smaller sequence number.
 *
 * CAREFUL HERE!!
 * We are doing a very silly assumption (which was told us by the TAs, so it is justifiable):
 * whenever we urb_deliver a message m5 with sequence number 5 and coming from process p1,
 * considering that we know that messages are broadcast in order, we can assume that whenever we
 * receive a message out of sequence, even the other messages with lower sequence number have been
 * broadcast in order, therefore we can deliver all messages up to the one we have received,
 * even if, strictly speaking, we have never received messages with that sequence number.
 *
 * For instance, we have received m5, but the last sequence number we have delivered so far for process p1
 * is 2. Then we can assume that process p1 has broadcast even 3 and 4 in the correct order, and therefore
 * we can deliver 3, 4 and finally 5, respecting the FIFO order, even if we haven't received yet nor 3 nor 4.
 *
 * @param fb
 */
void handle_urb_delivered(FifoBroadcast *fb) {
    while (true) {
        b_message msg = fb->get_next_urb_delivered();

        /// Make a very silly assumption:
        // every time you receive a message, you can assume that you can deliver all messages with
        // a lower sequence number starting from the same sender.
        if (fb->next_to_deliver[msg.first_sender] <= msg.seq_number) {
            for (int i = fb->next_to_deliver[msg.first_sender]; i <= msg.seq_number; i++) {
                // create an instance of the message to deliver, setting the correct sequence number
                b_message msg_to_fifo_deliver(i, msg.first_sender, msg.lcob_m);
                fb->fb_deliver(msg_to_fifo_deliver);
            }

            fb->next_to_deliver[msg.first_sender] = msg.seq_number + 1;  // increment the local counter by 1
        }
    }
}