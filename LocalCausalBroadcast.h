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

};


#endif //DISTRIBUTED_ALGORITHMS_LOCALCAUSALBROADCAST_H
