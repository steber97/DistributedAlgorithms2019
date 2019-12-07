
#include "LocalCausalBroadcast.h"







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
            lcob->graph->nodes[{msg.first_sender, msg.seq_number}] = new Node<T>(true,
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



/**
 * The message can be delivered when the number of unmet dependencies is 0
 * @return
 */
template <typename T>
bool Node<T>::can_be_delivered() {
    return this->unmet_dependencies == 0;
}


/**
 * Create an instance of Node.
 */
template <typename T>
Node<T>::Node(bool received, Graph<T> *graph, int proc, int seq_number,
              vector<int>& local_vc, lcob_message msg, LocalCausalBroadcast<T>* lcob) {
    this->process = proc;
    this->seq_number = seq_number;
    this->delivered = false;   // the message is not delivered by default
    this->received = received;   // we may have created a new node either when receiving a new message,
    // either when we create the dependency nodes (received = false)
    this->graph = graph;
    this->lcob = lcob;
    this->unmet_dependencies = 0;

    if (received == true){
        this->msg = msg;    // the message is true only when creating it after having received the message
        for (size_t i = 1; i<msg.vc.size(); i++){   // the vector clock is longer by 1
            if (msg.vc[i] > local_vc[i]){  // i is the process number, vector_clock[i] the sequence number
                // we have a dependency
                if (graph->nodes.find({i, msg.vc[i]}) == graph->nodes.end() ){
                    // we need to insert a new node
                    // CAREFUL HERE: vector_clock is not right! should be read only when received == true;
                    // the same for lcob_message
                    graph->nodes[{i, msg.vc[i]}] = new Node(false, graph, i, msg.vc[i],
                                                            this->lcob->local_vc, this->msg, this->lcob);
                }
                // Add the dependency
                graph->nodes[{i, msg.vc[i]}]->add_dependency(this);
                unmet_dependencies ++;   // one more unmet dependency
            }
        }
    }


}

template <typename T>
void Node<T>::add_dependency(Node<T> *node) {
    this->is_dependency_of.push_back(node);
}


/**
 * Tricky part:
 * when delivering the message, we must first check if we can deliver it.
 * If we can't, then we stop immediately.
 * If we can, we deliver it. Then, when we finish, we follow all the is_dependency_of, and try to deliver recursevly all
 * of them (and subtract one from the unmet_dependency)
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
        this->is_dependency_of[i]->delete_one_dependency();

        // the function already checks for the possibility to deliver
        this->is_dependency_of[i]->deliver_recursively(local_vc);
    }


}

template <typename T>
void Node<T>::delete_one_dependency() {
    // just remove one from the unmet_dependencies
    this->unmet_dependencies --;
}


template <typename T>
void Node<T>::update_existing_node(lcob_message& msg, vector<int>& local_vc) {
    this->msg = msg;
    this->received = true;

    for (size_t i = 1; i<msg.vc.size(); i++){   // the vector clock is longer by 1
        if (msg.vc[i] > this->lcob->local_vc[i]){  // i is the process number, vector_clock[i] the sequence number
            // we have a dependency
            if (graph->nodes.find({i, msg.vc[i]}) == graph->nodes.end() ){
                // we need to insert a new node
                // CAREFUL HERE: vector_clock is not right! should be read only when received == true;
                // the same for lcob_message
                graph->nodes[{i, msg.vc[i]}] = new Node<T>(false, graph, i, msg.vc[i],
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

    this->local_vc.resize(number_of_processes+1, 0);
    this->dependencies = dependencies;
    this->process_number = process_number;
    this->interface = interface;
    this->graph = new Graph<T>();
}

template <typename T>
void LocalCausalBroadcast<T>::lcob_broadcast(lcob_message &lcob_msg) {
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

template<typename T>
void LocalCausalBroadcast<T>::lcob_deliver(lcob_message &msg_to_deliver) {
    // log the delivery of the message
    lcob_delivery_log(msg_to_deliver);
}

template<>
lcob_message LocalCausalBroadcast<FifoBroadcast>::get_next_delivered(FifoBroadcast *fifo) {
    // this gets the message from the interface.
    // can be either fifo or urb. (depends on the template T).
    return this->interface->get_next_delivered();
}

template<>
lcob_message LocalCausalBroadcast<UrBroadcast>::get_next_delivered(UrBroadcast *urb) {
    // this gets the message from the interface.
    // can be either fifo or urb. (depends on the template T).
    return this->interface->get_next_delivered().lcob_m;
}


// actually instantiate the classes that I need
template class LocalCausalBroadcast<FifoBroadcast>;
template class LocalCausalBroadcast<UrBroadcast>;

template class Node<FifoBroadcast>;
template class Node<UrBroadcast>;

template class Graph<FifoBroadcast>;
template class Graph<UrBroadcast>;



