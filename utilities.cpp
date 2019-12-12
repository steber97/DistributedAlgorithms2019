#include "utilities.h"

// these are used to cover the logging functions
mutex mtx_log;
vector<string> log_actions;

// This is the mutex used to communicate with the shared queue between the urb and the beb.
mutex mtx_beb_urb;
queue<b_message> queue_beb_urb;
bool queue_beb_urb_locked = false;
condition_variable cv_beb_urb;


mutex mtx_pp2p_sender, mtx_pp2p_receiver, mtx_pp2p_get_msg;
bool stop_pp2p_receiver = false, stop_pp2p_sender = false, stop_pp2p_get_msg = false;

ofstream out1("log1");
ofstream out2("log2");
ofstream out3("log3");

/**
 * Parses the input file
 * @param membership_file the file to parse
 * @return a pair of:
 *          - the total number of message to be sent by each process
 *          - a map that contains info on ip and port for each process
 */
unordered_map<int, pair<string, int>> * parse_input_data(string &membership_file) {
    unordered_map<int, pair<string, int>> *socket_by_process_id = new(unordered_map<int, pair<string, int>>);
    ifstream mem_in(membership_file);
    int number_of_processes;
    mem_in >> number_of_processes;
    int pr_n;
    string ip;
    int port;
    for (int i = 0; i < number_of_processes; i++) {
        mem_in >> pr_n;
        mem_in >> ip;
        mem_in >> port;
        (*socket_by_process_id)[pr_n] = {ip, port};
    }
    return socket_by_process_id;
}


/**
 * A fake message is a message with a sequence number equal to -1
 * @param msg
 * @return
 */
bool is_pp2p_fake(pp2p_message msg){
    return msg.seq_number == -1;
}

pp2p_message create_fake_pp2p(){
    b_message fake_payload(-1, -1);
    return pp2p_message(false, -1LL, 0, fake_payload);
}

/**
 * emulates the python split function
 * @param s the string to split
 * @param c the char used to perform the split. must be a char (len==1)!
 * @return the same result of python split method
 */
vector<string>* split(const string& s, char c){
    vector<string>* res = new vector<string>();
    char mystr[s.size()+1];
    int posw = 0;
    for (size_t i = 0; i<s.size(); i++){
        if (s[i] == c){
            mystr[posw] = '\0';
            string temp (mystr, posw);
            res->push_back(temp);
            posw = 0;   // start from the beginning
        }
        else{
            mystr[posw] = s[i];
            posw ++;
        }
    }
    // stop the last one, as if we have found a terminating character at the end
    mystr[posw] = '\0';
    string temp (mystr, posw);
    res->push_back(temp);

    return res;
}

/**
 * The message format sent at the perfect link layer is :
 * perfect_link/broadcast/lcob
 * perfect_link: ack-process_num
 * broadcast: original_sender-seq_number
 * lcob: vc
 *
 * the vc is formatted as el1,el2,el3, ... ,eln
 *
 * for instance, the message
 * 0-3-1-1000/5-3/1,6,0,6,8  means a normal message (ack = 1) sent by process 3 with sequence number pp2p 1 and timestamp 1000,
 * which brings the message 3 originally sent by 5.
 * which has vector clock 1,6,0,6,8, which means that before being able to deliver that message,
 * the receiver must have delivered first message 1 from 1, message 6 from 2 and so on.
 * (Careful the position, they start from 0 here, but must be indexed by 1)
 * The len of the vc is strictly equal to the number of processes.
 * @param msg
 * @return
 */
pp2p_message parse_message(const string &msg) {

    vector<string>* cont_outer;
    char delim_outer = '/';
    // We need to have a message with size 2 (is like making the split by '/' in python).
    cont_outer = split(msg, delim_outer);


    // Parse the perfect link message
    vector<string>* cont1;
    char delim = '-';
    cont1 = split(cont_outer->at(0), delim);

    bool ack = stoi(cont1->at(0));
    int proc_number = stoi(cont1->at(1));
    long long seq_number_pp2p = stoll(cont1->at(2));
    unsigned long int timestamp = stoul(cont1->at(3));

    // parse the ur_broadcast message.
    vector<string>* cont2;
    delim = '-';
    cont2 = split(cont_outer->at(1), delim);

    int sender = stoi(cont2->at(0));
    int seq_number_broad = stoi(cont2->at(1));

    /*
    // parse the vector clock!
    vector<string>* cont3;
    delim = ',';
    cont3 = split(cont_outer->at(2), delim);

    vector<int> vector_clock;
    for (size_t i = 0; i < cont3->size(); i++){
        vector_clock.push_back(stoi(cont3->at(i)));
    }
    lcob_message lcob_m (seq_number_broad, sender, vector_clock);
     */
    b_message urb_msg(seq_number_broad, sender);
    pp2p_message pp2p_msg(ack, seq_number_pp2p, proc_number, timestamp, urb_msg);

    delete(cont_outer);
    delete(cont1);
    delete(cont2);
    //delete(cont3);

    return pp2p_msg;
}


/**
 * returns a string of the format:
 * 0-1-3/5-6
 * the first part (before /) is the perfect link message
 * the second is the uniform broadcast part
 *
 * ack - process - seq_number (its long long) / original_sender - sequence_number (without whitespaces)
 * @param msg
 * @return
 */
string to_string(pp2p_message &msg){
    return (msg.ack ? string("1") : string("0")) + "-" + to_string(msg.proc_number) + "-" + to_string(msg.seq_number) + "-" + to_string(msg.timestamp)
                + "/" + to_string(msg.payload.first_sender) + "-" + to_string(msg.payload.seq_number);
}


/**
 * Appends the broadcast log to the list of activities.
 * @param msg the broadcast message to log
 */
void urb_broadcast_log(b_message& msg) {
    string log_msg = "b " + to_string(msg.seq_number) ;
    mtx_log.lock();
    // Append the broadcast log message
    log_actions.push_back(log_msg);
    mtx_log.unlock();
}


/**
 * Appends the broadcast delivery to the list of activities.
 * @param msg
 * @param sender
 * @return
 */
void urb_delivery_log(b_message& msg) {
    string log_msg = "d " + to_string(msg.first_sender) + " " + to_string(msg.seq_number);
    mtx_log.lock();
    log_actions.push_back(log_msg);
    mtx_log.unlock();
}


void log(string s, int proc_num) {
    switch(proc_num) {
        case 1:
            out1 << s << endl;
            break;
        case 2:
            out2 << s << endl;
            break;
        case 3:
            out3 << s << endl;
            break;
        default:
            break;
    }
}