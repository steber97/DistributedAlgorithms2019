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



class Graph;

class Node {
private:
    int id;  // this is the process. Probably useless. It is the position in the graph
    bool received;
    bool delivered;
    int unmet_dependencies;
    vector<Node*> is_dependency_of;
    Graph* graph;
    int seq_number;
    int process;
    lcob_message msg;
    LocalCausalBroadcast<UrBroadcast>* lcob;

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
    Node(bool received, Graph *graph, int proc, int seq_number,
         vector<int>& local_vc, lcob_message msg, LocalCausalBroadcast<UrBroadcast>* lcob);

/**
 * Adds a dependency to the is_dependency_of vector.
 * @param node
 */
    void add_dependency(Node* node);

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

class Graph{
    // order process, message -> node
public:
    unordered_map<pair<int, int>, Node*, pair_hash> nodes;
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
    Graph* graph;

    int process_number;
    LocalCausalBroadcast<T>(int number_of_processes, int number_of_messages, T* interface, vector<vector<int>>* dependencies, const int process_number){
        this->number_of_processes = number_of_processes;
        this->number_of_messages = number_of_messages;

        this->local_vc.resize(number_of_processes+1, 0);
        this->dependencies = dependencies;
        this->process_number = process_number;
        this->interface = interface;
        this->graph = new Graph();
    }

    void lcob_broadcast(lcob_message &lcob_msg) {
        // before broadcasting it, you need to update the vc for this message.
        vector<int> vc (this->local_vc.size(), 0);  // copy the vc. set to zero all the entries for which we are not dependent.

        for (size_t i = 0; i < this->dependencies->at(this->process_number).size(); i++){
            // set to values != 0 only the processes we depend on.
            vc[dependencies->at(this->process_number)[i]] = this->local_vc[dependencies->at(this->process_number)[i]];
        }
        vc[dependencies->at(this->process_number)[0]] = lcob_msg.seq_number - 1;   // set the dependency on the previous message of the same sender (FIFO property).
        lcob_msg.vc = vc;
        b_message bMessage(lcob_msg.seq_number, lcob_msg.first_sender, lcob_msg);
        lcob_broadcast_log(lcob_msg);

        this->interface->broadcast(bMessage);  // either with FIFO or URB
    }

    void lcob_deliver(lcob_message &msg_to_deliver) {
        // log the delivery of the message
        lcob_delivery_log(msg_to_deliver);
    }

    lcob_message get_next_delivered(FifoBroadcast* fifo){
        // this gets the message from the interface.
        // can be either fifo or urb. (depends on the template T).
        return this->interface->get_next_delivered();
    }

    lcob_message get_next_delivered(UrBroadcast* urb){
        // this gets the message from the interface.
        // can be either fifo or urb. (depends on the template T).
        return this->interface->get_next_delivered().lcob_m;
    }

    void init();  // init method, defined below.
};

/**
 * This is used to build lcob on top of fifo
 * @param lcob
 */
template<typename T>
void handle_delivered_lcob(LocalCausalBroadcast<T> *lcob);

template<typename T>
void LocalCausalBroadcast<T>::init() {  // init method to spawn the thread that will handle the fifo delivery
    // This can both use the urb or fifo handler!
    thread t_delivered_handler(handle_delivered_lcob<T>, this);
    t_delivered_handler.detach();

}


/**
 * We can use as template the broadcast we want to use to build on top of it
 * Local Causal Order Broadcast.
 *
 * Can either be URB or FIFO.
 * The important thing is that both FIFO and URB implement a method broadcast
 * (wrapper for fb_broadcast or ur_broadcast).
 * @tparam T
 * @param lcob
 */
template<typename T>
void handle_delivered_lcob(LocalCausalBroadcast<T> *lcob){
    while (true) {
        lcob_message msg = lcob->get_next_delivered(lcob->interface);
        // lcob->lcob_deliver(msg);   // up to now deliver immediately, as if we are doing fifo
        // first check if the message can be delivered immediately (vc is OK)


        // first check if the message already exists in the graph:
        if (lcob->graph->nodes.find({msg.first_sender, msg.seq_number}) == lcob->graph->nodes.end()){
            lcob->graph->nodes[{msg.first_sender, msg.seq_number}] = new Node(true,
                    lcob->graph, msg.first_sender, msg.seq_number,
                    lcob->local_vc, msg, lcob);
        }
        else{
            // otherwise update the
            lcob->graph->nodes[{msg.first_sender, msg.seq_number}]->update_existing_node(msg, lcob->local_vc);
        }


        lcob->graph->nodes[{msg.first_sender, msg.seq_number}]->deliver_recursively(lcob->local_vc);
    }
}





#endif //DISTRIBUTED_ALGORITHMS_LOCALCAUSALBROADCAST_H
