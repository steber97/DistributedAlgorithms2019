#include <unistd.h>
#include <sstream>
#include "utilities.h"

// these are used to cover the logging functions
mutex mtx_log;
vector<string> log_actions;

atomic<bool> sigkill(false);

/**
 * Parses the input file
 * @param membership_file the file to parse
 * @return a pair of:
 *          - a map which maps to each process number its address and port
 *          - a matrix of dependencies (indexed by process)
 */
pair<unordered_map<int, pair<string, int>> *, vector<vector<int>>*> parse_input_data(string &membership_file) {
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
    string line;
    getline(mem_in, line);   // dunno why, looks like it reads the end of the last line.
    /// this part here is used to get localized broadcast dependent data:
    vector<vector<int>>* dependencies = new vector<vector<int>>(number_of_processes+1, vector<int>());
    for (int i = 1; i<=number_of_processes; i++){
        char sep = ' ';
        getline(mem_in, line);
        vector<string>* dep = split(line, sep);
        for (size_t j = 0; j<dep->size(); j++){
            dependencies->at(i).push_back(stoi(dep->at(j)));
        }
    }

    return {socket_by_process_id, dependencies};
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
 * 0-3-1/5-3/1,6,0,6,8  means a normal message (ack = 1) sent by process 3 with sequence number pp2p 1,
 * which brings the message 3 originally sent by 5.
 * which has vector clock 1,6,0,6,8, which means that before being able to deliver that message,
 * the receiver must have delivered first message 1 from 1, message 6 from 2 and so on.
 * (Careful the position, they start from 0 here, but must be indexed by 1)
 * The len of the vc is strictly equal to the number of processes.
 * @param msg
 * @return
 */
pp2p_message parse_message(const string &msg, const int process_number) {

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

    // parse the ur_broadcast message.
    vector<string>* cont2;
    delim = '-';
    cont2 = split(cont_outer->at(1), delim);

    int sender = stoi(cont2->at(0));
    int seq_number_broad = stoi(cont2->at(1));

    // parse the vector clock!
    vector<string>* cont3;
    delim = ',';
    cont3 = split(cont_outer->at(2), delim);

    vector<int> vector_clock;
    for (size_t i = 0; i < cont3->size(); i++){
        vector_clock.push_back(stoi(cont3->at(i)));
    }
    lcob_message lcob_m (seq_number_broad, sender, vector_clock);
    b_message urb_msg(seq_number_broad, sender, lcob_m);

    // I think the destination process when receiving the message is just useless, its the process itself.
    // But we will set it for clarity.
    pp2p_message pp2p_msg(ack, seq_number_pp2p, proc_number, process_number, urb_msg);

    delete(cont_outer);
    delete(cont1);
    delete(cont2);
    delete(cont3);

    return pp2p_msg;
}


/**
 * returns a string of the format:
 * 0-1-3/5-6/0,4,2,1
 * the first part (before /) is the perfect link message
 * the second is the uniform broadcast part
 * the third is the vector clock
 * ack - process - seq_number (its long long) / original_sender - sequence_number (without whitespaces) / vector clock as comma separated list
 * @param msg
 * @return
 */
string to_string(pp2p_message &msg){
    string vc_string;
    size_t size = msg.payload.lcob_m.vc.size();
    for (size_t i = 0; i < size; i++){
        // create the vc string, the comma mustn't be put a the first character
        vc_string += (i != 0 ? "," : "") + to_string(msg.payload.lcob_m.vc[i]);
    }

    return (msg.ack ? string("1") : string("0")) + "-" + to_string(msg.proc_number) + "-" + to_string(msg.seq_number)   /* this is the pp2p message */
                + "/" + to_string(msg.payload.first_sender) + "-" + to_string(msg.payload.seq_number)       /* this is the broadcast msg */
                + "/" + vc_string;        // this is the vector clock
}

/**
 * Checks whether the message is stop message!
 * We deliver stop messages when we close the connection and bad things happen!
 * @return
 */
bool is_pp2p_stop_message(pp2p_message &msg){
    return msg.seq_number == -1;
}

bool is_b_stop_message(b_message &bmessage) {
    return bmessage.seq_number == -1;
}

bool is_lcob_stop_message(lcob_message &lcob_message) {
    return lcob_message.seq_number == -1;
}

/**
 * Create a stop pp2p message, use only when closing the connection, in order to
 * stop even higher layers.
 */
pp2p_message create_stop_pp2p_message(const int number_of_processes){

    b_message stop_payload = create_stop_bmessage(number_of_processes);
    return pp2p_message(false, -1LL, -1, -1, stop_payload);
}

b_message create_stop_bmessage(const int number_of_processes) {
    lcob_message lcobMessage = create_stop_lcobmessage(number_of_processes);
    b_message stop_payload(-1, -1, lcobMessage);
    return stop_payload;
}

lcob_message create_stop_lcobmessage(const int number_of_processes) {
    vector<int> fake_vc(number_of_processes, INT32_MAX);   // Initialize it with stupid big numbers, so that it is not delivered as vc is too big!
    lcob_message lcobMessage (-1, -1, fake_vc);
    return lcobMessage;
}

