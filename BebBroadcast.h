//
// Created by stefano on 07/11/19.
//

#ifndef DISTRIBUTED_ALGORITHMS_BEBBROADCAST_H
#define DISTRIBUTED_ALGORITHMS_BEBBROADCAST_H

#include "utilities.h"
#include "Link.h"

class BebBroadcast {
public:
    Link* link;
    int number_of_processes;

    BebBroadcast(Link* link, int number_of_processes);

    void beb_broadcast(message m);

    void beb_delivery(broadcast_message m);

    void init();
};

void run_deliverer_beb(Link* link, BebBroadcast* beb_broadcast);

#endif //DISTRIBUTED_ALGORITHMS_BEBBROADCAST_H
