#include "BeBroadcast.h"


BeBroadcast::BeBroadcast(Link* link, int number_of_processes, int number_of_messages){
    this->link = link;
    this->number_of_processes = number_of_processes;
    this->number_of_messages = number_of_messages;
}


void BeBroadcast::beb_broadcast(message &msg) {
    for (int i = 1; i <= number_of_processes; i++) {
        if (number_of_processes != link->get_process_number())
            link->send_to(i, msg);
    }
}


void BeBroadcast::beb_deliver(message &msg) {
    (*acks)[msg.seq_number - 1].insert(msg.proc_number);
    if (forward->find(msg.seq_number) == forward->end()) {
        forward->insert(msg.seq_number);
        beb_broadcast(msg);
    }
}


