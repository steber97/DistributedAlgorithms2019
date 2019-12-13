
#include "LocalCausalBroadcast.h"


/**
 * This is used to build lcob on top of fifo
 * @param lcob
 */
template<typename T>
void handle_delivered_lcob(LocalCausalBroadcast<T> *lcob);

/**
 * Detach the thread which gets new delivered messages from lower abstractions.
 */
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

        if (stop_pp2p || is_lcobmessage_fake(msg)){
            break;   // time to stop!
        }
        // lcob->lcob_deliver(msg);   // up to now deliver immediately, as if we are doing fifo
        // first check if the message can be delivered immediately (vc is OK)


        // first check if the message already exists in the graph:
        if (lcob->graph->nodes.find({msg.first_sender, msg.seq_number}) == lcob->graph->nodes.end()){
            lcob->graph->nodes[{msg.first_sender, msg.seq_number}] = new Node<T>(true,
                                                                                 lcob->graph,
                                                                                 lcob->local_vc, msg, lcob);
        }
        else{
            // otherwise update the
            lcob->graph->nodes[{msg.first_sender, msg.seq_number}]->update_existing_node(msg, lcob->local_vc);
        }
        lcob->graph->nodes[{msg.first_sender, msg.seq_number}]->deliver_recursively(lcob->local_vc);
    }

    delete(lcob->graph);
}



/**
 * States if a message can be delivered or not
 * unmet_dep == 0
 * @return
 */
template <typename T>
bool Node<T>::can_be_delivered() {
    return this->unmet_dependencies == 0;
}


/**
 * This method must build the node.
 * delivered is certainly False, received can be either true either false.
 * count the unmet dependencies (through the vector_clock)
 * and set edges from its dependencies to itself.
 * Careful, vector_clock can be empty/wrong (when received == false).
 * @param id
 * @param vector_clock
 */
template <typename T>
Node<T>::Node(bool received, Graph<T> *graph,
              vector<int>& local_vc, lcob_message msg, LocalCausalBroadcast<T>* lcob) {

    this->delivered = false;   // the message is not delivered by default
    this->received = received;   // we may have created a new node either when receiving a new message,
    // either when we create the dependency nodes (received = false)
    this->graph = graph;
    this->lcob = lcob;
    this->unmet_dependencies = 0;

    if (received){
        // the message is true only when creating it after having received the message.
        // otherwise it is just a fake message
        this->msg = msg;
        for (size_t i = 1; i<msg.vc.size(); i++){   // the vector clock is longer by 1
            // Do we have a dependency?
            if (msg.vc[i] > local_vc[i]){  // i is the process number, vector_clock[i] the sequence number
                // Do we need to insert a new node?
                if (graph->nodes.find({i, msg.vc[i]}) == graph->nodes.end() ){
                    // CAREFUL HERE: vector_clock is not right! should be read only when received == true;
                    // the same for lcob_message
                    graph->nodes[{i, msg.vc[i]}] = new Node(false, graph,
                                                            this->lcob->local_vc, this->msg, this->lcob);
                }
                // Add the dependency
                graph->nodes[{i, msg.vc[i]}]->add_dependency(this);
                unmet_dependencies ++;   // one more unmet dependency
            }
        }
    }


}

/**
 * Adds a dependency to the is_dependency_of vector.
 * @param node
 */
template <typename T>
void Node<T>::add_dependency(Node<T> *node) {
    // Just insert the node in the adjacency vector.
    this->is_dependency_of.push_back(node);
}


/**
 * Tricky part:
 * when delivering the message, we must first check if we can deliver it.
 * If we can't, then we stop immediately.
 * If we can, we deliver it. Then, when we finish, we follow all the
 * is_dependency_of, and try to deliver recursively
 * all of them (and subtract one from the unmet_dependency)
 */
template <typename T>
void Node<T>::deliver_recursively(vector<int>& local_vc) {
    if (!this->can_be_delivered()){
        // can't do anything
        return;
    }

    // deliver the message
    this->delivered = true;
    this->lcob->lcob_deliver(this->msg);
    // as vector clock is passed as a reference, it gets updated even in the LocalCausalBroadcast class
    local_vc[this->msg.first_sender] ++;   // increment the vector clock

    for (size_t i = 0; i< this->is_dependency_of.size(); i++){
        // try recursively to deliver all dependencies!
        // first remove one unmet_dependency (we have just delivered a new message)
        this->is_dependency_of[i]->delete_one_dependency();

        // the function already checks for the possibility to deliver
        this->is_dependency_of[i]->deliver_recursively(local_vc);
    }


}


/**
 * subtract 1 from unmet_dependencies
 */
template <typename T>
void Node<T>::delete_one_dependency() {
    // just subtract one from the unmet_dependencies
    this->unmet_dependencies --;
}


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
template <typename T>
void Node<T>::update_existing_node(lcob_message& msg, vector<int>& local_vc) {
    // we have finally a message for that node.
    this->msg = msg;
    this->received = true;

    for (size_t i = 1; i<msg.vc.size(); i++){   // the vector clock is longer by 1
        if (msg.vc[i] > this->lcob->local_vc[i]){  // i is the process number, vector_clock[i] the sequence number
            // we have a dependency
            if (graph->nodes.find({i, msg.vc[i]}) == graph->nodes.end() ){
                // we need to insert a new node
                // CAREFUL HERE: vector_clock is not right! should be read only when received == true;
                // the same for lcob_message
                graph->nodes[{i, msg.vc[i]}] = new Node<T>(false, graph,
                                                           this->lcob->local_vc, this->msg, this->lcob);
            }
            // Add the dependency
            graph->nodes[{i, msg.vc[i]}]->add_dependency(this);
            unmet_dependencies ++;   // one more unmet dependency
        }
    }
}


template <typename T>
LocalCausalBroadcast<T>::LocalCausalBroadcast(int number_of_processes, int number_of_messages, T* interface, vector<vector<int>>* dependencies, const int process_number){
    this->number_of_processes = number_of_processes;
    this->number_of_messages = number_of_messages;

    this->local_vc.resize(number_of_processes+1, 0);  // set to zero all initial dependencies
    this->dependencies = dependencies;
    this->process_number = process_number;
    this->interface = interface;
    this->graph = new Graph<T>();
}


/**
 * Broadcast a message in Local Causal Order Broadcast
 * Before broadcasting it with the lower layer, it needs to create the
 * vector clock for the message (copying the entries of the local vector clock
 * the process depends on).
 * @param lcob_msg
 */
template <typename T>
void LocalCausalBroadcast<T>::lcob_broadcast(lcob_message &lcob_msg) {
    // before broadcasting it, you need to update the vc for this message.
    vector<int> vc (this->local_vc.size(), 0);  // copy the vc. set to zero all the entries for which we are not dependent.

    for (size_t i = 0; i < this->dependencies->at(this->process_number).size(); i++){
        // set to values != 0 only the processes we depend on.
        vc[dependencies->at(this->process_number)[i]] = this->local_vc[dependencies->at(this->process_number)[i]];
    }
    vc[this->process_number] = lcob_msg.seq_number - 1;   // set the dependency on the previous message of the same sender (FIFO property).
    lcob_msg.vc = vc;
    b_message bMessage(lcob_msg.seq_number, lcob_msg.first_sender, lcob_msg);
    broadcast_log<lcob_message>(lcob_msg);

    this->interface->broadcast(bMessage);  // either with FIFO or URB
}


/**
 * Just log the delivery of the message, there is no higher layer
 * to deliver the message to! :)
 * @param msg_to_deliver
 */
template<typename T>
void LocalCausalBroadcast<T>::lcob_deliver(lcob_message &msg_to_deliver) {
    // log the delivery of the message
    delivery_log<lcob_message>(msg_to_deliver);
}


/**
 * Get next message from the lower layer Fifo.
 * @param fifo
 * @return lcob message delivered by fifo broadcast in fifo order (first in the queue first out).
 * fifo broadcast and fifo order have nothing to do with each other
 */
template<>
lcob_message LocalCausalBroadcast<FifoBroadcast>::get_next_delivered(FifoBroadcast *fifo) {
    // this gets the message from the interface.
    // can be either fifo or urb. (depends on the template T).
    return this->interface->get_next_delivered();
}

/**
 * Get next message from the lower layer Ur Broadcast.
 * @param urb
 * @return lcob message delivered by Ur broadcast in fifo order (first in the queue first out)
 */
template<>
lcob_message LocalCausalBroadcast<UrBroadcast>::get_next_delivered(UrBroadcast *urb) {
    // this gets the message from the interface.
    // can be either fifo or urb. (depends on the template T).
    return this->interface->get_next_delivered().lcob_m;
}


// actually instantiate the classes that I need
// it is done in this fancy way otherwise the template gets broken!
template class LocalCausalBroadcast<FifoBroadcast>;
template class LocalCausalBroadcast<UrBroadcast>;

template class Node<FifoBroadcast>;
template class Node<UrBroadcast>;

template class Graph<FifoBroadcast>;
template class Graph<UrBroadcast>;



