#include "UrBroadcast.h"

condition_variable cv_urb_delivering_queue; //
mutex mtx_urb_delivering_queue;             // To handle concurrency on the queue of the messages delivered at urb level
bool urb_delivering_queue_locked = false;   //


UrBroadcast::UrBroadcast(BeBroadcast *beb, int number_of_processes, int number_of_messages) {
    this->beb = beb;
    this->number_of_processes = number_of_processes;
    this->number_of_messages = number_of_messages;
    this->urb_delivering_queue = new queue<b_message>;
}


/**
 * This method initializes a thread that handle messages delivered at beb level
 */
void UrBroadcast::init() {
    thread delivery_checker(handle_beb_delivery, this);
    delivery_checker.detach();
}

/**
 * The actual method that broadcasts messages at urb level
 *
 * @param msg
 */
void UrBroadcast::urb_broadcast(b_message &msg)  {
    //urb_broadcast_log(msg);
    this->mtx_forward.lock();
    this->forward.insert({msg.first_sender, msg.seq_number});
    this->mtx_forward.unlock();

    beb->beb_broadcast(msg);
}


/**
 * Method used to when a message has to be delivered at urb level,
 * this is pushed into a queue that contains all the delivered messages at urb level
 * @param msg
 */
void UrBroadcast::urb_deliver(b_message &msg) {
    unique_lock<mutex> lck(mtx_urb_delivering_queue);
    cv_urb_delivering_queue.wait(lck, [&] { return !urb_delivering_queue_locked; });
    urb_delivering_queue_locked = true;
    this->urb_delivering_queue->push(msg);
    urb_delivering_queue_locked = false;
    cv_urb_delivering_queue.notify_all();

    //urb_delivery_log(msg);
}


/**
 * Gets the next message delivered by urb, by popping it from the queue
 * It's used by the fifo level to get the messages delivered at the inferior level
 *
 * @return the head of urb_delivering_queue
 */
b_message UrBroadcast::get_next_urb_delivered() {
    unique_lock<mutex> lck(mtx_urb_delivering_queue);
    cv_urb_delivering_queue.wait(lck, [&] { return !urb_delivering_queue->empty(); });
    urb_delivering_queue_locked = true;
    b_message next_message = this->urb_delivering_queue->front();
    this->urb_delivering_queue->pop();
    urb_delivering_queue_locked = false;
    cv_urb_delivering_queue.notify_all();
    return next_message;
}


/**
 * Checks whether a message has been delivered or not
 *
 * @return true if delivered
 *         false otherwise
 */
bool UrBroadcast::is_delivered(b_message &msg) {
    this->mtx_delivered.lock();
    bool is_delivered = (this->delivered.find({msg.first_sender, msg.seq_number}) != delivered.end());
    this->mtx_delivered.unlock();
    return is_delivered;
}


/**
 * Checks how many acks (at urb level) have been received for a specific message
 *
 * @param msg the message for which we want to check the number of received acks
 * @return the number of received acks
 */
int UrBroadcast::acks_received(b_message &msg) {
    this->mtx_acks.lock();
    int n_acks = this->acks[{msg.first_sender, msg.seq_number}];
    this->mtx_acks.unlock();
    return n_acks;
}


/**
 * Adds a message to the set of the delivered messages
 *
 * @param msg message to add to the set
 */
void UrBroadcast::addDelivered(b_message &msg) {
    this->mtx_delivered.lock();
    this->delivered.insert({msg.first_sender, msg.seq_number});
    this->mtx_delivered.unlock();
}


int UrBroadcast::get_number_of_processes() {
    return number_of_processes;
}

void UrBroadcast::broadcast(b_message &msg) {
    // This is just a wrapper for the ur_broadcast, so that it can be
    // invoked by lcob_broadcast.
    this->urb_broadcast(msg);
}

b_message UrBroadcast::get_next_delivered() {
    // just a wrapper with the same name as in fifo, so that can be used as template in Local Causal Reliable Broadcast.
    return this->get_next_urb_delivered();
}


/**
 * This method gets messages from the shared queue between BEB and URB and handle them
 * It is the way URB can interact with BEB.
 *
 * @param urb
 */
void handle_beb_delivery(UrBroadcast *urb) {
    while (true) {
        // First retrieves the message from the queue.
        unique_lock<mutex> lck(mtx_beb_urb);
        cv_beb_urb.wait(lck, [&] { return !queue_beb_urb.empty(); });
        queue_beb_urb_locked = true;
        b_message msg = queue_beb_urb.front();
        queue_beb_urb.pop();
        queue_beb_urb_locked = false;
        cv_beb_urb.notify_one();

        // Add the message to acked ones.
        urb->mtx_acks.lock();
        if (urb->acks.find({msg.first_sender, msg.seq_number}) == urb->acks.end()) {
            // first time we receive it.
            urb->acks[{msg.first_sender, msg.seq_number}] = 0;
        }
        urb->acks[{msg.first_sender, msg.seq_number}]++;
        urb->mtx_acks.unlock();

        // forward it if the process has never forwarded it before.
        urb->mtx_forward.lock();
        if (urb->forward.find({msg.first_sender, msg.seq_number}) == urb->forward.end()) {
            // We have never forwarded it before.
            urb->forward.insert({msg.first_sender, msg.seq_number});
            urb->mtx_forward.unlock();
            urb->beb->beb_broadcast(msg);
        } else
            urb->mtx_forward.unlock();

        // deliver the message if we have received enough acks.
        if (!urb->is_delivered(msg)) {  // is_delivered already takes into account concurrency issues.
            int acks_received = urb->acks_received(
                    msg);  // acks received already takes into account concurrency issues.
            if (acks_received >= ((urb->get_number_of_processes() / 2) + 1)) {
                // We have received enough acks for this message,
                // therefore we can deliver it!
                urb->addDelivered(msg);
                urb->urb_deliver(msg);
            }
        }
    }
}



