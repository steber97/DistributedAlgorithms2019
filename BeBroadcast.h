#include <thread>

#include "utilities.h"
#include "Link.h"

#ifndef DISTRIBUTED_ALGORITHMS_BEBROADCAST_H
#define DISTRIBUTED_ALGORITHMS_BEBROADCAST_H


class BeBroadcast {
private:
    Link* link;
    int number_of_processes;
    int number_of_messages;
public:
    BeBroadcast(Link* link, int number_of_processes, int number_of_messages);
    void beb_deliver(message &msg);
    void beb_broadcast(message &msg);
};


#endif //DISTRIBUTED_ALGORITHMS_BEBROADCAST_H
