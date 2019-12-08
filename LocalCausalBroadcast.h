#ifndef DISTRIBUTED_ALGORITHMS_LOCALCAUSALBROADCAST_H
#define DISTRIBUTED_ALGORITHMS_LOCALCAUSALBROADCAST_H

#include "FifoBroadcast.h"
#include "UrBroadcast.h"
#include "utilities.h"

using namespace std;

/**
 * This is a hasher used to hash lcob messages.
 * In this way, we can create a hashtable which has as keys messages.
 */
struct LCOBHasher{
    size_t
    operator()(const lcob_message & obj) const
    {
        return obj.first_sender * 100000 + obj.seq_number;
    }
};

/**
 * Comparator for LCOB messages,
 * overrides == for messages
 */
struct LCOBComparator
{
    bool
    operator()(const lcob_message & obj1, const lcob_message & obj2) const
    {
        if ((obj1.seq_number == obj2.seq_number) && (obj1.first_sender == obj2.first_sender))
            return true;
        return false;
    }
};

template <typename T>   // T stands for FifoBroadcast or UrBroadcast
class LocalCausalBroadcast;

template <typename T>
class Graph;

template<typename T>
class Node {
private:
    int id;  // this is the process. Probably useless. It is the position in the graph

    // boolean that states if the message has actually been received.
    // it may happen, in fact, that we create a Node for a message without the message being actually arrived.
    // This happens, for instance, when we receive a message that has as dependency a message which hasn't
    // been received yet.
    bool received;

    bool delivered;

    // the number of unmet dependencies for a particular message.
    // Whenever it gets to 0, we can deliver the message.
    int unmet_dependencies;

    /*
     * This stores, for every message m1, which other messages can be delivered after having delivered m1.
     * It is useful because as soon as we deliver m1, we follow all the is_dependency_of and subtract 1 from
     * unmet dependencies.
     */
    vector<Node*> is_dependency_of;
    Graph<T>* graph;
    lcob_message msg;
    LocalCausalBroadcast<T>* lcob;

    /**
     * States if a message can be delivered or not
     * unmet_dep == 0
     * @return
     */
    bool can_be_delivered();


public:
    /**
     * This method must build the node.
     * delivered is certainly False, received can be either true either false.
     * count the unmet dependencies (through the vector_clock)
     * and set edges from its dependencies to itself.
     * Careful, vector_clock can be empty/wrong (when received == false).
     * @param id
     * @param vector_clock
     */
    Node<T>(bool received, Graph<T> *graph,
            vector<int>& local_vc, lcob_message msg, LocalCausalBroadcast<T>* lcob);

    /**
     * Adds a dependency to the is_dependency_of vector.
     * @param node
     */
    void add_dependency(Node<T>* node);

    /**
     * Tricky part:
     * when delivering the message, we must first check if we can deliver it.
     * If we can't, then we stop immediately.
     * If we can, we deliver it. Then, when we finish, we follow all the
     * is_dependency_of, and try to deliver recursively
     * all of them (and subtract one from the unmet_dependency)
     */
    void deliver_recursively(vector<int>& vc);

    /**
     * subtract 1 from unmet_dependencies
     */
    void delete_one_dependency();

    /**
     * Update an existing node.
     * When creating the node without having received the actual message, we can invoke this method
     * to update all the useful missing information
     * For instance, it sets received to true, creates the edges to itself and
     * computes unmet_dependencies
     *
     * @param msg
     * @param local_vc
     */
    void update_existing_node(lcob_message& msg, vector<int>& local_vc);
};



template <typename T>
class Graph{
    // order process, message -> node
public:
    unordered_map<pair<int, int>, Node<T>*, pair_hash> nodes;
};


/**
 * Local Causal Order Broadcast
 * can be built both on FIFO (and it respects the FIFO property automagically)
 * or on top of Urb (with a small caveat:
 * every process must set for every message m1 as dependency with itself m1 - 1)
 * Of course Urb works slightly faster, as it is a lower abstraction.
 * @tparam T Fifo or Urb Broadcast class
 */
template <typename T>   // T stands for FifoBroadcast or UrBroadcast
class LocalCausalBroadcast {

private:
    int number_of_processes, number_of_messages;

public:
    T* interface;     // can be either fifo or urb

    // the local vector clock. Stores up until what message we have received from every process.
    vector<int> local_vc;

    // The vector of dependencies, per each process.
    vector<vector<int>>* dependencies;
    Graph<T>* graph;

    int process_number;

    LocalCausalBroadcast<T>(int number_of_processes, int number_of_messages, T* interface, vector<vector<int>>* dependencies, const int process_number);

    /**
     * Broadcast a message in Local Causal Order Broadcast
     * Before broadcasting it with the lower layer, it needs to create the
     * vector clock for the message (copying the entries of the local vector clock
     * the process depends on).
     * @param lcob_msg
     */
    void lcob_broadcast(lcob_message &lcob_msg) ;

    /**
     * Just log the delivery of the message, there is no higher layer
     * to deliver the message to! :)
     * @param msg_to_deliver
     */
    void lcob_deliver(lcob_message &msg_to_deliver);

    /**
     * Get next message from the lower layer Fifo.
     * @param fifo
     * @return lcob message delivered by fifo broadcast in fifo order (first in the queue first out).
     * fifo broadcast and fifo order have nothing to do with each other
     */
    lcob_message get_next_delivered(FifoBroadcast* fifo);

    /**
     * Get next message from the lower layer Ur Broadcast.
     * @param urb
     * @return lcob message delivered by Ur broadcast in fifo order (first in the queue first out)
     */
    lcob_message get_next_delivered(UrBroadcast* urb);

    /**
     * Detach the thread which gets new delivered messages from lower abstractions.
     */
    void init();  // init method, defined below.
};



#endif //DISTRIBUTED_ALGORITHMS_LOCALCAUSALBROADCAST_H
