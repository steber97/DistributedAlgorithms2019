
#include "BebBroadcast.h"


void BebBroadcast::beb_broadcast(broadcast_message m){
    /**
     * It only sends the payload of the message to everybody.
     */
    create_broadcast_log(m);

    for (int i = 1; i <= this->number_of_processes; i++) {
        link->send_to(i, m.payload);
    }
}

void BebBroadcast::beb_delivery(broadcast_message m){
    // at the beb level, we only deliver the message!
    create_delivery_log(m);
}


BebBroadcast::BebBroadcast(Link * link, int number_of_processes) {
    this->link = link;
    this->number_of_processes = number_of_processes;
}


void BebBroadcast::init(){
    // starts the deliverer
    thread delivery_checker(run_deliverer_beb, this->link, this);
    delivery_checker.detach();
}


/**
 * The deliverer only receives messages from the link level
 * using the interface get_next_message.
 * It runs on a separate thread
 * @param link
 * @param number_of_processes
 */
void run_deliverer_beb(Link* link, BebBroadcast* beb_broadcast){
    while(true) {
        message msg = link->get_next_message();
        // the broadcast message that the beb delivery gets
        // is with same sender and seq number of the pp2p message.
        broadcast_message bm(msg.seq_number, msg.proc_number, msg);
        beb_broadcast->beb_delivery(bm);
    }
}