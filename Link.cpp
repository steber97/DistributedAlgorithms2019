#include "Link.h"

atomic<bool> stop_link_daemons(false);

//These three data structures contain the incoming messages, the messages to resend and the outgoing acks
BlockingReaderWriterQueue<pp2p_message> incoming_messages(1000);
BlockingReaderWriterQueue<pair<int, pp2p_message>> outgoing_acks(1000);
BlockingReaderWriterQueue<pair<int, pp2p_message>> messages_to_resend(1000);

bool outgoing_messages_locked = false;
mutex mtx_outgoing_messages;
condition_variable cv_outgoing_messages;
queue<pair<int, pp2p_message>> outgoing_messages;

mutex mtx_acks;
vector<unordered_set<long long int>> acks;  // acks received
vector<unordered_set<long long int>> pl_delivered;  //sequence numbers of the delivered messages at perfect link level
                                                    //ordered by the process that sent them


Link::Link(int sockfd, int process_number, unordered_map<int, pair<string, int>> *socket_by_process_id) {
    // Constructor
    this->sockfd = sockfd;
    this->process_number = process_number;
    this->socket_by_process_id = socket_by_process_id;
    this->last_seq_number.resize(this->socket_by_process_id->size() + 1, 0LL);  // Initialize all sequence numbers to zero.
}


/**
 * This method spawns the threads that will be used to send and receive messages on a port
 */
void Link::init() {
    thread t_rec(run_receiver, this);
    thread t_send(run_sender, this->socket_by_process_id, this->sockfd);
    thread t_ack_enq(run_ack_enqueuer);
    thread t_resend(run_resender);
    t_rec.detach();
    t_send.detach();
    t_ack_enq.detach();
    t_resend.detach();
}


int Link::get_sockfd(){
    return this->sockfd;
}


int Link::get_process_number() {
    return this->process_number;
}


/**
 * Format of messages:
 * - 1 bit for ack: 0 normal message, 1 ack.
 *
 * CAREFUL HERE: the message is not ready to run, first the sequence number must be added.
 *
 * This method only puts the message in the queue of outgoing messages, so that the sender can send it.
 * @param d_process_number destination address.
 * @param msg the message to send
 */
void Link::send_to(int d_process_number, pp2p_message& msg) {
    // We keep a queue of messages to send and not acked yet.
    // Sending a pp2p_message consists easily in putting a new pp2p_message in the
    // queue of outgoing messages, then the sender will send it.

    // Before doing it, we must choose a proper sequence number.
    long long seq_number = this->last_seq_number[d_process_number] ++;
    msg.seq_number = seq_number;
    unique_lock<mutex> lck(mtx_outgoing_messages);
    cv_outgoing_messages.wait(lck, []{ return !outgoing_messages_locked; });
    outgoing_messages_locked = true;
    outgoing_messages.push({d_process_number, msg});
    outgoing_messages_locked = false;
    cv_outgoing_messages.notify_all();
}


/**
 * Method used to send an acks at link level, in other words when a message is received an ack is sent to the broadcaster
 */
void Link::send_ack(pp2p_message &msg) {
    // Create the ack string (1-source process-sequence number of pp2p_message to ack)
    pp2p_message ack_message(true, msg.seq_number, process_number, msg.payload);
    outgoing_acks.enqueue({msg.proc_number, ack_message});
}

/**
 * This method gets the incoming messages and then handles both the sending of the ack and the delivery at perfect link level.
 *
 * It avoids duplicate messages.
 * @return the next new message.
 */
pp2p_message Link::get_next_message(){
    pp2p_message msg = create_fake_pp2p();
    while(true) {
        incoming_messages.wait_dequeue(msg);

        if (stop_link_daemons.load())
            return msg;

#ifdef DEBUG
        cout << process_number << " ha ricevuto (" << msg.proc_number << ", " << msg.seq_number << ")" << endl;
#endif
        if (msg.ack) {
            // than we received an ack;
            mtx_acks.lock();
            acks[msg.proc_number].insert(msg.seq_number);
            mtx_acks.unlock();
        } else {
            this->send_ack(msg);
            // check if it has not been delivered already
            if (pl_delivered[msg.proc_number].find(msg.seq_number) == pl_delivered[msg.proc_number].end()) {
                pl_delivered[msg.proc_number].insert(msg.seq_number);
                return msg;
            }
        }
    }
}


/**
 * Procedure run by the thread sender, it dequeues outgoing messages and then sends
 *
 * @param socket_by_process_id  data structure that maps every process (number) to its (ip, port) pair
 * @param sockfd the socket fd of the link's owner process
 */
void run_sender(unordered_map<int, pair<string, int>>* socket_by_process_id, int sockfd) {
    struct sockaddr_in d_addr;
    while (true) {
        unique_lock<mutex> lck(mtx_outgoing_messages);
        cv_outgoing_messages.wait(lck, [] { return !outgoing_messages.empty(); });

        if (stop_link_daemons.load()) {
            cv_outgoing_messages.notify_all();
            break;
        }

        outgoing_messages_locked = true;
        pair<int, pp2p_message> dest_and_msg = outgoing_messages.front();
        outgoing_messages.pop();
        outgoing_messages_locked = false;
        cv_outgoing_messages.notify_all();

        bool is_ack = dest_and_msg.second.ack;

#ifdef DEBUG
        cout << "inviando (" << dest_and_msg.second.proc_number << ", " << dest_and_msg.second.seq_number << ") a " << dest_and_msg.first << endl;
#endif

        if (is_ack || !is_acked(dest_and_msg.first, dest_and_msg.second.seq_number)) {

            string msg_s = to_string(dest_and_msg.second);
            const char *msg_c = msg_s.c_str();

            memset(&d_addr, 0, sizeof(d_addr));
            d_addr.sin_family = AF_INET;
            d_addr.sin_port = (*socket_by_process_id)[dest_and_msg.first].second;
            string ip_address = (*socket_by_process_id)[dest_and_msg.first].first;
            inet_pton(AF_INET, ip_address.c_str(), &(d_addr.sin_addr));

            sendto(sockfd, msg_c, strlen(msg_c),
                   MSG_CONFIRM, (const struct sockaddr *) &d_addr,
                   sizeof(d_addr));

            if (!is_ack)
                messages_to_resend.enqueue(dest_and_msg);
        }
    }

    cout << "Stopped sender" << endl;
}


/**
 * This procedure is used to pick ack from the SRSW queue of outgoing acks and push them in the queue of outgoing messages
 */
void run_ack_enqueuer() {
    while (true) {
        pair<int, pp2p_message> outgoing_ack(-1, create_fake_pp2p());
        outgoing_acks.wait_dequeue(outgoing_ack);

        if (stop_link_daemons.load()) break;

        unique_lock<mutex> lck(mtx_outgoing_messages);
        cv_outgoing_messages.wait(lck, []{ return !outgoing_messages_locked; });

        if (stop_link_daemons.load()) {
            cv_outgoing_messages.notify_all();
            break;
        }

        outgoing_messages_locked = true;
        outgoing_messages.push(outgoing_ack);
        outgoing_messages_locked = false;
        cv_outgoing_messages.notify_all();
    }

    cout << "Stopped ack enqueuer" << endl;
}


/**
 * This is the procedure run by the thread receiver, it listens on a certain port until it gets a message and then it
 * puts the message the incoming_messages queue
 *
 * @param link
 */
void run_receiver(Link *link) {
    while (true) {
        unsigned int len;
        char buf[1024];
        struct sockaddr_in sender_addr;
        int n = recvfrom(link->get_sockfd(), (char *)buf, MAXLINE, MSG_WAITALL, ( struct sockaddr *) &sender_addr,
                         &len);

        if (stop_link_daemons.load()) break;

        buf[n] = '\0';
        pp2p_message msg = parse_message(string(buf));

#ifdef DEBUG
        cout << link->get_process_number() << " ha ricevuto il messaggio (" << msg.proc_number << ", " << msg.seq_number << ")" << endl;
#endif
        // Put the message in the queue.
        incoming_messages.enqueue(msg);
    }

    cout << "Stopped receiver" << endl;
}


void run_resender() {
    while (true) {
        pair<int, pp2p_message> msg_to_resend(-1, create_fake_pp2p());
        messages_to_resend.wait_dequeue(msg_to_resend);

        if (stop_link_daemons.load()) break;

        unique_lock<mutex> lck(mtx_outgoing_messages);
        cv_outgoing_messages.wait(lck, []{ return !outgoing_messages_locked; });

        if (stop_link_daemons.load()) {
            cv_outgoing_messages.notify_all();
            break;
        }

        outgoing_messages_locked = true;
        outgoing_messages.push(msg_to_resend);
        outgoing_messages_locked = false;
        cv_outgoing_messages.notify_all();

        usleep(50);
    }

    cout << "Stopped resender" << endl;
}


bool is_acked(int proc_number, long long seq_number) {
    bool acked;
    mtx_acks.lock();
    acked = acks[proc_number].find(seq_number) != acks[proc_number].end();
    mtx_acks.unlock();
    return acked;
}