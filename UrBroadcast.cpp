#include "UrBroadcast.h"

condition_variable cv_urb_delivering_queue; //
mutex mtx_urb_delivering_queue;             // To handle concurrency on the queue of the messages delivered at urb level
bool urb_delivering_queue_locked = false;   //


UrBroadcast::UrBroadcast(BeBroadcast *beb, int number_of_processes, int number_of_messages) {
    this->beb = beb;
    this->number_of_processes = number_of_processes;
    this->number_of_messages = number_of_messages;
}


/**
 * This method detaches a new thread which will listen to incoming beb delivered messages.
 */
void UrBroadcast::init() {
    thread delivery_checker(handle_beb_delivery, this);
    delivery_checker.detach();
}

/**
 * broadcast the message to all processes.
 * Before doing so, it needs to store the message in the forwarded set.
 * Otherwise, it could risk to broadcast it more than once.
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
 * Delivers the message putting it in the queue of urb delivered messages.
 * @param msg
 */
void UrBroadcast::urb_deliver(b_message &msg) {
    unique_lock<mutex> lck(mtx_urb_delivering_queue);
    cv_urb_delivering_queue.wait(lck, [&] { return !urb_delivering_queue_locked; });
    urb_delivering_queue_locked = true;
    this->urb_delivering_queue.push(msg);
    urb_delivering_queue_locked = false;
    cv_urb_delivering_queue.notify_all();

    // Log the delivery of the message
    //urb_delivery_log(msg);
}


/**
 * Returns the next message that has been urb delivered, reading from
 * the queue urb_delivering_queue (it is the reader method, the writer instead is
 * handle_beb_delivery).
 * @return the first (fifo order, it is a queue) urb delivered message
 */
b_message UrBroadcast::get_next_urb_delivered() {
    unique_lock<mutex> lck(mtx_urb_delivering_queue);
    cv_urb_delivering_queue.wait(lck, [&] { return !urb_delivering_queue.empty() || stop_pp2p; });
    if (stop_pp2p){
        b_message fake = create_fake_bmessage(this->number_of_processes);
        return fake;
    }
    urb_delivering_queue_locked = true;
    b_message next_message = this->urb_delivering_queue.front();
    this->urb_delivering_queue.pop();
    urb_delivering_queue_locked = false;
    cv_urb_delivering_queue.notify_all();
    return next_message;
}


/**
 * Check whether the message has already been delivered, looking in the set
 * delivered. Otherwise, it could happen that any message is delivered more than once.
 * @param msg
 * @return
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
 * It manages automatically the concurrency
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


/**
 * just an alias for urb_broadcast so that it can be used with templates
 * @param msg
 */
void UrBroadcast::broadcast(b_message &msg) {
    // This is just a wrapper for the ur_broadcast, so that it can be
    // invoked by lcob_broadcast.
    this->urb_broadcast(msg);
}


/**
 * Just an alias for get_next_urb_delivered, so that it can be used with templates.
 * @return get_next_urb_delivered()
 */
b_message UrBroadcast::get_next_delivered() {
    // just a wrapper with the same name as in fifo, so that can be used as template in Local Causal Reliable Broadcast.
    return this->get_next_urb_delivered();
}


/**
 * Gets beb delivered messages and decides if it is the case to urb_deliver them.
 * A message which has been beb_delivered can't immediately be urb_delivered: in fact
 * the first time a new message arrives it needs to be forwarded to all other processes.
 * Moreover, we store how many times we see the same message repeated (as every process
 * forwards every message, there are going to be possibly n copies of the same message
 * going around, where n is the number of processes).
 * As soon as we have received n/2 + 1 acks, it means that we can urb_deliver it.
 *
 * @param urb
 */
void handle_beb_delivery(UrBroadcast *urb) {
    while (true) {
        // First retrieves the message from beb
        b_message msg = urb->beb->get_next_beb_delivered();

        if (stop_pp2p || is_bmessage_fake(msg))
            // time to stop
            break;

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



