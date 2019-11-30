#ifndef DISTRIBUTED_ALGORITHMS_LOCALCAUSALBROADCAST_H
#define DISTRIBUTED_ALGORITHMS_LOCALCAUSALBROADCAST_H

#include "FifoBroadcast.h"
#include "utilities.h"


class LocalCausalBroadcast {

private:
    int number_of_processes, number_of_messages;
    FifoBroadcast* fb;

public:
    LocalCausalBroadcast(int number_of_processes, int number_of_messages, FifoBroadcast* fb);
    void lcob_broadcast(lcob_message& lcob_msg);
    void lcob_deliver(lcob_message &msg_to_deliver);
    lcob_message get_next_fifo_delivered();
    void init();  // init method to spawn the thread that will handle the fifo delivery

};

void handle_fifo_delivered(LocalCausalBroadcast *lcob);

#endif //DISTRIBUTED_ALGORITHMS_LOCALCAUSALBROADCAST_H
