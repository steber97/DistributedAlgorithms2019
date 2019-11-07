#include "BeBroadcast.h"

BeBroadcast::BeBroadcast(Link* link, int number_of_processes, int number_of_messages){
    this->link = link;
    this->number_of_processes = number_of_processes;
    this->number_of_messages = number_of_messages;
}


void BeBroadcast::init(){
    // starts the deliverer
    thread delivery_checker(run_deliverer_beb, this->link, this);
    delivery_checker.detach();
}


void BeBroadcast::beb_broadcast(broadcast_message &msg) {
    message pp2p_msg = message(false, this->link->get_process_number(), msg);
    for (int i = 1; i <= number_of_processes; i++) {
        link->send_to(i, pp2p_msg);
    }
}


void BeBroadcast::beb_deliver(broadcast_message &msg) {
    // Put the message in the queue so that it can be delivered to urb.
    unique_lock<mutex> lck(mtx_beb_urb);
    cv_beb_urb.wait(lck, [&] { return !queue_beb_urb_locked; });
    queue_beb_urb_locked = true;
    queue_beb_urb.push(msg);
    queue_beb_urb_locked = false;
    cv_beb_urb.notify_one();
}


/**
 * The deliverer only receives messages from the link level
 * using the interface get_next_message.
 * It runs on a separate thread
 * @param link
 * @param number_of_processes
 */
void run_deliverer_beb(Link* link, BeBroadcast* beb_broadcast){
    while(true) {
        message msg = link->get_next_message();
        // the broadcast message that the beb delivery gets
        // is with same sender and seq number of the pp2p message.
        broadcast_message bm(msg.payload);
        beb_broadcast->beb_deliver(bm);

    }
}

