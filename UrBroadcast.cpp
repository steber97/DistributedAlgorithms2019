
#include "UrBroadcast.h"


UrBroadcast::UrBroadcast(BeBroadcast *beb, int number_of_processes, int number_of_messages) {
    this->beb = beb;
    this->number_of_processes = number_of_processes;
    this->number_of_messages = number_of_messages;
}


void UrBroadcast::init() {
    thread delivery_checker(handle_beb_delivery, this);
    delivery_checker.detach();
}


void UrBroadcast::urb_broadcast(broadcast_message &msg) {
    broadcast_log(msg);   // This already takes into account concurrency
    this->mtx_forward.lock();
    forward.insert({msg.sender, msg.seq_number});
    this->mtx_forward.unlock();

    beb->beb_broadcast(msg);
}

void UrBroadcast::urb_deliver(broadcast_message &msg) {
    delivery_log(msg);
}

bool UrBroadcast::is_delivered(broadcast_message &msg) {
    this->mtx_delivered.lock();
    bool is_delivered = (this->delivered.find({msg.sender, msg.seq_number}) != delivered.end());
    this->mtx_delivered.unlock();
    return is_delivered;
}


int UrBroadcast::acks_received(broadcast_message &msg) {
    this->mtx_acks.lock();
    int n_acks = this->acks[{msg.sender, msg.seq_number}];
    this->mtx_acks.unlock();
    return n_acks;
}


void UrBroadcast::addDelivered(broadcast_message &msg) {
    this->mtx_delivered.lock();
    this->delivered.insert({msg.sender, msg.seq_number});
    this->mtx_delivered.unlock();
}


int UrBroadcast::get_number_of_processes() {
    return number_of_processes;
}


void handle_beb_delivery(UrBroadcast *urb) {
    /**
     * This method gets messages from the shared queue among BEB and URB
     * It is the way URB can interact with BEB.
     */
    while (true) {
        // First retrieves the message from the queue.
        unique_lock<mutex> lck(mtx_beb_urb);
        cv_beb_urb.wait(lck, [&] { return !queue_beb_urb.empty(); });
        queue_beb_urb_locked = true;
        broadcast_message msg = queue_beb_urb.front();
        queue_beb_urb.pop();
        queue_beb_urb_locked = false;
        cv_beb_urb.notify_one();

        // Add the message to acked ones.
        urb->mtx_acks.lock();
        if (urb->acks.find({msg.sender, msg.seq_number}) == urb->acks.end()) {
            // first time we receive it.
            urb->acks[{msg.sender, msg.seq_number}] = 0;
        }
        urb->acks[{msg.sender, msg.seq_number}]++;
        urb->mtx_acks.unlock();

        // forward it if the process has never forwarded it before.
        urb->mtx_forward.lock();
        if (urb->forward.find({msg.sender, msg.seq_number}) == urb->forward.end()) {
            // We have never forwarded it before.
            urb->forward.insert({msg.sender, msg.seq_number});
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



