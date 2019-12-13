#include "BeBroadcast.h"

// This is the mutex used to communicate with the shared queue between the urb and the beb.
mutex mtx_beb_urb;
queue<b_message> queue_beb_urb;
bool queue_beb_urb_locked = false;
condition_variable cv_beb_urb;


BeBroadcast::BeBroadcast(Link* link, int number_of_processes, int number_of_messages){
    this->link = link;
    this->number_of_processes = number_of_processes;
    this->number_of_messages = number_of_messages;
}

/**
 * Initialization phase, it detaches a new thread which stays forever listening to
 * new messages from perfect links.
 */
void BeBroadcast::init(){
    // starts the deliverer on a separate detached thread
    thread delivery_checker(handle_link_delivered, this->link, this);
    delivery_checker.detach();
}

/**
 * Broadcasts a message to all processes.
 * @param msg the message to broadcast.
 */
void BeBroadcast::beb_broadcast(b_message &msg) {
    // Just send the message to every process.

    for (int i = 1; i <= number_of_processes; i++) {
        pp2p_message pp2p_msg = pp2p_message(false, this->link->get_process_number(), i, msg);
        // we do not specify here the sequence number! It will be set by Link!
        link->send_to(i, pp2p_msg);
    }
}


/**
 * Delivers a message to the upper layer (in this case, Urb).
 * @param msg the message to beb deliver
 */
void BeBroadcast::beb_deliver(b_message &msg) {
    // Put the message in the queue so that it can be delivered to urb.
    unique_lock<mutex> lck(mtx_beb_urb);
    cv_beb_urb.wait(lck, [&] { return !queue_beb_urb_locked; });
    queue_beb_urb_locked = true;
    queue_beb_urb.push(msg);
    queue_beb_urb_locked = false;
    cv_beb_urb.notify_one();
}


/**
 * Reads concurrently the first message that can be beb_delivered.
 * Read is concurrent as the thread detached with init() is the writer,
 * and threads running the upper layers are the readers (actually only one, Urb)
 * It can be blocking, if no messages are present in the queue, for instance,
 * it waits using a condition variable, until there is a new message to beb deliver.
 *
 * @return the first message which has been beb_delivered
 */
b_message BeBroadcast::get_next_beb_delivered(){
    // acquire the lock on the condition variable, shared with the thread running handle_link_delivered
    unique_lock<mutex> lck(mtx_beb_urb);
    cv_beb_urb.wait(lck, [&] { return !queue_beb_urb.empty() || sigkill; });
    if (sigkill){
        return create_stop_bmessage(this->number_of_processes);
    }
    queue_beb_urb_locked = true;
    b_message msg = queue_beb_urb.front();  // push the message in the queue
    queue_beb_urb.pop();
    queue_beb_urb_locked = false;
    cv_beb_urb.notify_one();    // notify the other thread that the queue now is ready to be written in

    return msg;
}



/**
 * This method runs on a separate thread, just listens to incoming messages coming from
 * the lower layer (perfect Link) and beb delivers them as soon as they arrive.
 *
 * Small caveat: perfect link sends stop messages when da_proc is killed
 * When it happens, stop messages are delivered by Link.
 *
 * @param link
 * @param beb_broadcast
 */
void handle_link_delivered(Link* link, BeBroadcast* be_broadcast){
    while(true) {
        pp2p_message msg = link->get_next_message();
        // the broadcast pp2p_message that the beb delivery gets
        // is with same first_sender and seq number of the pp2p pp2p_message.
        if (!sigkill and !is_pp2p_stop_message(msg)) {  // only deliver it if it is not stop_message.
            be_broadcast->beb_deliver(msg.payload);
        }
        else {
            // Time to stop the detached thread
            break;
        }
    }
}

