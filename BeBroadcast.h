#include <thread>

#include "utilities.h"
#include "Link.h"

#ifndef DISTRIBUTED_ALGORITHMS_BEBROADCAST_H
#define DISTRIBUTED_ALGORITHMS_BEBROADCAST_H

/**
 * This class represent the best-effort broadcast abstraction
 */
class BeBroadcast {
private:
    Link* link;
    int number_of_processes;
    int number_of_messages;
public:
    BeBroadcast(Link* link, int number_of_processes, int number_of_messages);
    void init();
    void beb_broadcast(b_message &msg);
    void beb_deliver(b_message &msg);
    b_message get_next_beb_delivered();
};

void run_deliverer_beb(Link* link, BeBroadcast* beb_broadcast);


#endif //DISTRIBUTED_ALGORITHMS_BEBROADCAST_H
