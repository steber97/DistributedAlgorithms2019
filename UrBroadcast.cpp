#include "UrBroadcast.h"

atomic<bool> stop_urb_daemon(false);

BlockingReaderWriterQueue<b_message> urb_delivering_queue(100);

UrBroadcast::UrBroadcast(BeBroadcast *beb, int number_of_processes, int number_of_messages) {
    this->beb = beb;
    this->number_of_processes = number_of_processes;
    this->number_of_messages = number_of_messages;
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
    urb_delivering_queue.enqueue(msg);

#ifdef DEBUG
    cout << "URB-delivered the message " << msg.seq_number << " sent by " << msg.first_sender << endl;
#endif
}


/**
 * Gets the next message delivered by urb, by popping it from the queue
 * It's used by the fifo level to get the messages delivered at the inferior level
 *
 * @return the head of urb_delivering_queue
 */
b_message UrBroadcast::get_next_urb_delivered() {
    b_message front;
    urb_delivering_queue.wait_dequeue(front);
    return front;
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


b_message UrBroadcast::get_next_beb_delivered() {
    return beb->get_next_beb_delivered();
}


/**
 * This method gets messages from the shared queue between BEB and URB and handle them
 * It is the way URB can interact with BEB.
 *
 * @param urb
 */
void handle_beb_delivery(UrBroadcast *urb) {
    b_message msg = urb->get_next_beb_delivered();
    while (!stop_urb_daemon.load()) {
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

        msg = urb->get_next_beb_delivered();
    }
}



