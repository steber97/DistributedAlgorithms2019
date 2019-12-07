#ifndef DISTRIBUTED_ALGORITHMS_LOCALCAUSALBROADCAST_H
#define DISTRIBUTED_ALGORITHMS_LOCALCAUSALBROADCAST_H

#include "FifoBroadcast.h"
#include "UrBroadcast.h"
#include "utilities.h"

using namespace std;

struct LCOBHasher{
    size_t
    operator()(const lcob_message & obj) const
    {
        return obj.first_sender * 10000 + obj.seq_number;
    }
};

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
    bool received;
    bool delivered;
    int unmet_dependencies;
    vector<Node*> is_dependency_of;
    Graph<T>* graph;
    int seq_number;
    int process;
    lcob_message msg;
    LocalCausalBroadcast<T>* lcob;

/**
 * Easily count the unmet dependencies
 * @return
 */
    bool can_be_delivered();


public:
/**
 * This method must build the node.
 * Set received to True, delivered to False
 * count the unmet dependencies (through the vector_clock)
 * and set edges from its dependencies to itself.
 * Careful, vector_clock can be empty/wrong (when received == false).
 * @param id
 * @param vector_clock
 */
    Node<T>(bool received, Graph<T> *graph, int proc, int seq_number,
            vector<int>& local_vc, lcob_message msg, LocalCausalBroadcast<T>* lcob);

/**
 * Adds a dependency to the is_dependency_of vector.
 * @param node
 */
    void add_dependency(Node<T>* node);

/**
 * Deliver recursively: when the node can be delivered,
 * it follows all is_dependency_of edges, and tries to deliver all
 * nodes that were waiting for him.
 * Set deliver to true
 */
    void deliver_recursively(vector<int>& vc);

    void delete_one_dependency();

    void update_existing_node(lcob_message& msg, vector<int>& local_vc);
};



template <typename T>
class Graph{
    // order process, message -> node
public:
    unordered_map<pair<int, int>, Node<T>*, pair_hash> nodes;
};



template <typename T>   // T stands for FifoBroadcast or UrBroadcast
class LocalCausalBroadcast {

private:
    int number_of_processes, number_of_messages;

public:
    T* interface;     // can be either fifo or urb
    vector<int> local_vc;
    unordered_set<lcob_message, LCOBHasher, LCOBComparator> pending;
    vector<vector<int>>* dependencies;
    Graph<T>* graph;

    int process_number;
    LocalCausalBroadcast<T>(int number_of_processes, int number_of_messages, T* interface, vector<vector<int>>* dependencies, const int process_number);

    void lcob_broadcast(lcob_message &lcob_msg) ;

    void lcob_deliver(lcob_message &msg_to_deliver);

    lcob_message get_next_delivered(FifoBroadcast* fifo);

    lcob_message get_next_delivered(UrBroadcast* urb);

    void init();  // init method, defined below.
};



#endif //DISTRIBUTED_ALGORITHMS_LOCALCAUSALBROADCAST_H
