#include "LocalCausalBroadcast.h"


/**
 * The message can be delivered when the number of unmet dependencies is 0
 * @return
 */
bool Node::can_be_delivered() {
    return this->unmet_dependencies == 0;
}


/**
 * Create an instance of Node.
 */
Node::Node(bool received, vector<int> &vector_clock, Graph *graph, int proc, int seq_number,
           vector<int>& local_vc, lcob_message msg, LocalCausalBroadcast<UrBroadcast>* lcob) {
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
        for (size_t i = 1; i<vector_clock.size(); i++){   // the vector clock is longer by 1
            if (vector_clock[i] > local_vc[i]){  // i is the process number, vector_clock[i] the sequence number
                // we have a dependency
                if (graph->nodes.find({i, vector_clock[i]}) == graph->nodes.end() ){
                    // we need to insert a new node
                    // CAREFUL HERE: vector_clock is not right! should be read only when received == true;
                    // the same for lcob_message
                    graph->nodes[{i, vector_clock[i]}] = new Node(false, vector_clock, graph, i, vector_clock[i],
                                                                  this->lcob->local_vc, this->msg, this->lcob);
                }
                // Add the dependency
                graph->nodes[{i, vector_clock[i]}]->add_dependency(this);
                unmet_dependencies ++;   // one more unmet dependency
            }
        }
    }


}

void Node::add_dependency(Node *node) {
    this->is_dependency_of.push_back(node);
}


/**
 * Tricky part:
 * when delivering the message, we must first check if we can deliver it.
 * If we can't, then we stop immediately.
 * If we can, we deliver it. Then, when we finish, we follow all the is_dependency_of, and try to deliver recursevly all
 * of them (and subtract one from the unmet_dependency)
 */
void Node::deliver_recursively(vector<int>& local_vc) {
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

void Node::delete_one_dependency() {
    // just remove one from the unmet_dependencies
    this->unmet_dependencies --;
}


void Node::update_existing_node(lcob_message& msg, vector<int>& local_vc) {
    this->msg = msg;
    this->received = true;

    for (size_t i = 1; i<msg.vc.size(); i++){   // the vector clock is longer by 1
        if (msg.vc[i] > this->lcob->local_vc[i]){  // i is the process number, vector_clock[i] the sequence number
            // we have a dependency
            if (graph->nodes.find({i, msg.vc[i]}) == graph->nodes.end() ){
                // we need to insert a new node
                // CAREFUL HERE: vector_clock is not right! should be read only when received == true;
                // the same for lcob_message
                graph->nodes[{i, msg.vc[i]}] = new Node(false, msg.vc, graph, i, msg.vc[i],
                                                              this->lcob->local_vc, this->msg, this->lcob);
            }
            // Add the dependency
            graph->nodes[{i, msg.vc[i]}]->add_dependency(this);
            unmet_dependencies ++;   // one more unmet dependency
        }
    }
}


