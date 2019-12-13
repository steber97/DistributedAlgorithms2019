#include <thread>

#include "utilities.h"
#include "Link.h"

#ifndef DISTRIBUTED_ALGORITHMS_BEBROADCAST_H
#define DISTRIBUTED_ALGORITHMS_BEBROADCAST_H



/**
 * These are used to handle the queue that is in the middle between beb and urb,
 * it contains the messages delivered by beb and that should be handled by urb
 */
extern queue<b_message> queue_beb_urb; // the actual queue

extern condition_variable cv_beb_urb;  //
extern mutex mtx_beb_urb;              // to handle concurrency on the queue
extern bool queue_beb_urb_locked;      //


/**
 * This class represents the best-effort broadcast abstraction
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
 * When it happens, stop messages are sent.
 *
 * @param link
 * @param be_broadcast
 */
void handle_link_delivered(Link* link, BeBroadcast* be_broadcast);


#endif //DISTRIBUTED_ALGORITHMS_BEBROADCAST_H
