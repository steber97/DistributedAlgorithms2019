//
// Created by orazio on 11/8/19.
//

#ifndef DISTRIBUTED_ALGORITHMS_FIFOBROADCAST_H
#define DISTRIBUTED_ALGORITHMS_FIFOBROADCAST_H


#include "UrBroadcast.h"

class FifoBroadcast {
private:
    UrBroadcast *urb;

public:
    //queue<pair<int, int>> pending; //set of pending messages stored as pair of int,
                                    //the first one is the process number of the sender,
                                    //the second one is the sequence number of the messag

    vector<unordered_set<int>> pending;   // indexed by sender and sequence number;
    vector<int> next_to_deliver;
    FifoBroadcast(UrBroadcast *urb,int number_of_processes);

    /**
     * Detaches the thread that will receives messages retrieved from lower layers (URB).
     */
    void init();

    /**
     * Fifo broadcast a message, is actually the same as urb broadcasting it.
     */
    void fb_broadcast(b_message fifo_msg);

    /**
     * Delivers a message to the upper layers
     * (messages are pushed into a queue of messages which can be accessed concurrently).
     * @param msg_to_deliver
     */
    void fb_deliver(b_message msg_to_deliver);

    /**
     * Just an alias for fb_broadcast, so that it can be used by template class
     */
    void broadcast(b_message);

    /**
     * Gets next message coming from the lower layer (urb)
     * @return
     */
    b_message get_next_urb_delivered();

    /**
     * method exposed for upper layers to get the next fifo delivered message.
     * Can be blocking, when there is no message to deliver.
     * Uses condition variables and notify in order not to waste resources doing busy waiting.
     * @return
     */
    lcob_message get_next_fifo_delivered();

    /**
     * just an alias for get_next_fifo_delivered()
     * @return
     */
    lcob_message get_next_delivered();

    queue<lcob_message*> *fifo_delivering_queue;
};


/**
 * Decides when it is the case to deliver messages coming from the lower layers.
 * It has to respect FIFO order, so whenever we receive messages from the same
 * process but out of order, we need to wait and deliver first the remaining messages
 * with a smaller sequence number.
 *
 * CAREFUL HERE!!
 * We are doing a very silly assumption (which was told us by the TAs, so it is justifiable):
 * whenever we urb_deliver a message m5 with sequence number 5 and coming from process p1,
 * considering that we know that messages are broadcast in order, we can assume that whenever we
 * receive a message out of sequence, even the other messages with lower sequence number have been
 * broadcast in order, therefore we can deliver all messages up to the one we have received,
 * even if, strictly speaking, we have never received messages with that sequence number.
 *
 * For instance, we have received m5, but the last sequence number we have delivered so far for process p1
 * is 2. Then we can assume that process p1 has broadcast even 3 and 4 in the correct order, and therefore
 * we can deliver 3, 4 and finally 5, respecting the FIFO order, even if we haven't received yet nor 3 nor 4.
 *
 * @param fb
 */
void handle_urb_delivered(FifoBroadcast *fb);

#endif //DISTRIBUTED_ALGORITHMS_FIFOBROADCAST_H
