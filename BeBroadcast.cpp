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


/**
 * The actual method that broadcast a message
 *
 * @param msg to be broadcasted
 */
void BeBroadcast::beb_broadcast(b_message &msg) {
    pp2p_message pp2p_msg = pp2p_message(false, this->link->get_process_number(), msg);
    for (int i = 1; i <= number_of_processes; i++) {
        link->send_to(i, pp2p_msg);
    }
}


/**
 * Method used when a message ha to be delivered, in fact it pushes
 * the message in the queue of the messages delivered at beb level
 *
 * @param msg message to be delivered
 */
void BeBroadcast::beb_deliver(b_message &msg) {
    // Put the message in the queue so that it can be delivered to urb.
    unique_lock<mutex> lck(mtx_beb_urb);
    cv_beb_urb.wait(lck, [&] { return !queue_beb_urb_locked; });
    queue_beb_urb_locked = true;
    queue_beb_urb.push(msg);
    queue_beb_urb_locked = false;
    cv_beb_urb.notify_one();

    // log the delivery of the message
    // urb_delivery_log(msg);
}


b_message BeBroadcast::get_next_beb_delivered(){
    unique_lock<mutex> lck(mtx_beb_urb);
    cv_beb_urb.wait(lck, [&] { return !queue_beb_urb.empty(); });
    queue_beb_urb_locked = true;
    b_message msg = queue_beb_urb.front();
    queue_beb_urb.pop();
    queue_beb_urb_locked = false;
    cv_beb_urb.notify_one();

    return msg;
}



/**
 * The deliverer only receives messages from the link level
 * using the method exposed by the link get_next_message.
 * It runs on a separate thread
 *
 * @param link
 * @param number_of_processes
 */
void run_deliverer_beb(Link* link, BeBroadcast* be_broadcast){
    while(true) {
        pp2p_message msg = link->get_next_message();
        // the broadcast pp2p_message that the beb delivery gets
        // is with same first_sender and seq number of the pp2p pp2p_message.
        if (!is_pp2p_fake(msg))   // only deliver it if it is not fake.
            be_broadcast->beb_deliver(msg.payload);
    }
}

