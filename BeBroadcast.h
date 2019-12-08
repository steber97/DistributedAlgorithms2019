#include <thread>

#include "utilities.h"
#include "Link.h"

#ifndef DISTRIBUTED_ALGORITHMS_BEBROADCAST_H
#define DISTRIBUTED_ALGORITHMS_BEBROADCAST_H

/**
 * This class represent the best-effort broadcast abstraction
 * It simply relies on the perfect link properties in order to fulfill
 * validity, no duplication and no creation.
 */
class BeBroadcast {
private:
    Link* link;
    int number_of_processes;
    int number_of_messages;
public:
    BeBroadcast(Link* link, int number_of_processes, int number_of_messages);

    /**
     * Initialization phase, it detaches a new thread which stays forever listening to
     * new messages from perfect links.
     */
    void init();

    /**
     * Broadcasts a message to all processes.
     * @param msg the message to broadcast.
     */
    void beb_broadcast(b_message &msg);

    /**
     * Delivers a message to the upper layer (in this case, Urb).
     * @param msg the message to beb deliver
     */
    void beb_deliver(b_message &msg);

    /**
     * Reads concurrently the first message that can be beb_delivered.
     * Read is concurrent as the thread detached with init() is the writer,
     * and threads running the upper layers are the readers (actually only one, Urb)
     * It can be blocking, if no messages are present in the queue, for instance,
     * it waits using a condition variable, until there is a new message to beb deliver.
     *
     * @return the first message which has been beb_delivered
     */
    b_message get_next_beb_delivered();
};


/**
 * This method runs on a separate thread, just listens to incoming messages coming from
 * the lower layer (perfect Link) and beb delivers them as soon as they arrive.
 *
 * Small caveat: sometimes perfect link sends garbage (it happens for instance during threads shutdown)
 * When it happens, fake messages are sent.
 *
 * @param link
 * @param beb_broadcast
 */
void run_deliverer_beb(Link* link, BeBroadcast* beb_broadcast);


#endif //DISTRIBUTED_ALGORITHMS_BEBROADCAST_H
