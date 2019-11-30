
#include "LocalCausalBroadcast.h"


LocalCausalBroadcast::LocalCausalBroadcast(int number_of_processes, int number_of_messages, FifoBroadcast *fb) {
    this->number_of_processes = number_of_processes;
    this->number_of_messages = number_of_messages;

    this->fb = fb;
}

void LocalCausalBroadcast::lcob_broadcast(lcob_message &lcob_msg) {
    // In the meantime just fifo broadcast it!
    b_message bMessage(lcob_msg.seq_number, lcob_msg.first_sender, lcob_msg);

    this->fb->fb_broadcast(bMessage);
}
