#ifndef DISTRIBUTED_ALGORITHMS_URBROADCAST_H
#define DISTRIBUTED_ALGORITHMS_URBROADCAST_H

#include <unordered_set>
#include <condition_variable>
#include <unordered_map>

#include "BeBroadcast.h"

using namespace std;


extern condition_variable cv_urb_delivering_queue;


class UrBroadcast {
private:
    int number_of_processes;
    int number_of_messages;
    queue<b_message> urb_delivering_queue;
public:

    // pointer to the lower level beb instance
    BeBroadcast *beb;

    // set that stores the delivered and forwarded messages (so that they are not delivered or forwarded twice)
    unordered_set<pair<int,int>, pair_hash> delivered; // both delivered and forward are indexed by sender, sequence number.
    unordered_set<pair<int,int>, pair_hash> forward;
    // an unordered map which maps, per each message (sender, seq_number)
    // the number of replicas received for the same message (like an ack)
    // When the number exceeds half the processes + 1, then the message can be
    // urb delivered
    unordered_map<pair<int,int>, int, pair_hash> acks;

    // mutexes to access forward, delivered and acks data structure concurrently.
    mutex mtx_forward, mtx_delivered, mtx_acks;


    UrBroadcast(BeBroadcast *beb, int number_of_processes, int number_of_messages);

    /**
     * This method detaches a new thread which will listen to incoming beb delivered messages.
     */
    void init();

    /**
     * broadcast the message to all processes.
     * Before doing so, it needs to store the message in the forwarded set.
     * Otherwise, it could risk to broadcast it more than once.
     * @param msg
     */
    void urb_broadcast(b_message &msg);

    /**
     * Delivers the message putting it in the queue of urb delivered messages.
     * @param msg
     */
    void urb_deliver(b_message &msg);

    /**
     * Returns the next message that has been urb delivered, reading from
     * the queue urb_delivering_queue (it is the reader method, the writer instead is
     * handle_beb_delivery).
     * @return the first (fifo order, it is a queue) urb delivered message
     */
    b_message get_next_urb_delivered();

    /**
     * Just an alias for get_next_urb_delivered, so that it can be used with templates.
     * @return get_next_urb_delivered()
     */
    b_message get_next_delivered();  //  a wrapper for the local_causal_reliable_broadcast.

    /**
     * just an alias for urb_broadcast so that it can be used with templates
     * @param msg
     */
    void broadcast(b_message &msg);

    /**
     * Check whether the message has already been delivered, looking in the set
     * delivered. Otherwise, it could happen that any message is delivered more than once.
     * @param msg
     * @return
     */
    bool is_delivered(b_message &msg);

    /**
     * Checks how many acks (at urb level) have been received for a specific message
     *
     * @param msg the message for which we want to check the number of received acks
     * @return the number of received acks
     */
    int acks_received(b_message &msg);


    int get_number_of_processes();

    /**
     * Adds a message to the set of the delivered messages
     * It manages automatically the concurrency
     * @param msg message to add to the set
     */
    void addDelivered(b_message &msg);
};


/**
 * Gets beb delivered messages and decides if it is the case to urb_deliver them.
 * A message which has been beb_delivered can't immediately be urb_delivered: in fact
 * the first time a new message arrives it needs to be forwarded to all other processes.
 * Moreover, we store how many times we see the same message repeated (as every process
 * forwards every message, there are going to be possibly n copies of the same message
 * going around, where n is the number of processes).
 * As soon as we have received n/2 + 1 acks, it means that we can urb_deliver it.
 *
 * @param urb
 */
void handle_beb_delivery(UrBroadcast* urb);

#endif //DISTRIBUTED_ALGORITHMS_URBROADCAST_H
