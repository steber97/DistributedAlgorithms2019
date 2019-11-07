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
    broadcast_log(msg);
    for (int i = 1; i <= number_of_processes; i++) {
        if (number_of_processes != link->get_process_number()){}
            //TODO decommenta e togli le graffe dopo l'IF ----- link->send_to(i, msg);
    }
}


void BeBroadcast::beb_deliver(broadcast_message &msg) {
    delivery_log(msg);
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

        // TODO: fix message constructor!!
        // broadcast_message bm(msg.seq_number, msg.proc_number, msg);
        // beb_broadcast->beb_deliver(bm);

    }
}

