#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <iostream>
#include "Link.h"
#include "utilities.h"
#include "Broadcast.h"

using namespace std;

static int wait_for_start = 1;
bool initialization_phase = true;

static void start(int signum) {
	wait_for_start = 0;
}

static void stop(int signum) {
	//reset signal handlers to default
	signal(SIGTERM, SIG_DFL);
	signal(SIGINT, SIG_DFL);

	//immediately stop network packet processing
	printf("Immediately stopping network packet processing.\n");

	//write/flush output file if necessary
	printf("Writing output.\n");

	//exit directly from signal handler
	exit(0);
}

int main(int argc, char** argv) {

    if (argc != 3){
        // wrong number of arguments
        cout << "Wrong arguments number!!" << endl;
        exit(EXIT_FAILURE);
    }

    //set signal handlers
    signal(SIGUSR2, start);
    signal(SIGTERM, stop);
    signal(SIGINT, stop);

    int process_number = atoi(argv[1]);

    cout << "The process id is " << ::getpid() << endl;

	//parse arguments, including membership
	//initialize application
	//start listening for incoming UDP packets

	string membership_file = argv[2];

	// input data contains both the number of messages to send per each process,
	// and the mapping among processes and ip/port
	pair<int, unordered_map<int, pair<string, int>>*> input_data = parse_input_data(membership_file);
    int number_of_processes = input_data.second->size();
    int number_of_messages = input_data.first;

    int sockfd;
    string ip_address = (*input_data.second)[process_number].first;
    int port = (*input_data.second)[process_number].second;
    // Creating socket file descriptor
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        cerr << "socket creation failed but maybe recovery";
    }
    memset(&(sock), 0, sizeof(sock));
    sock.sin_family = AF_INET; // IPv4
    inet_pton(AF_INET, ip_address.c_str(), &(sock.sin_addr));
    sock.sin_port = port;
    if ( bind(sockfd, (const struct sockaddr *)&(sock),
              sizeof(sock)) < 0 ) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    Link* link = new Link(sockfd, process_number, input_data.second);
    Broadcast* broadcast = new Broadcast(link, number_of_processes, number_of_messages);

    //wait until start signal
	while(wait_for_start) {
		struct timespec sleep_time;
		sleep_time.tv_sec = 0;
		sleep_time.tv_nsec = 1000;
		nanosleep(&sleep_time, NULL);
	}

    link->init();
	broadcast->init();


	cout << "init finished" << endl;

    //broadcast messages
    printf("Broadcasting messages.\n");

    for (int i = 1; i <= number_of_messages; i++) {
        message msg;
        msg.ack = false;
        msg.seq_number = i;
        msg.proc_number = process_number;
        msg.payload = to_string(i);
        broadcast->urb_broadcast(msg);
    }

	/*

	// Try to send a lot of messages at the time.
	for (int i = 1; i<=number_of_processes; i++){
        if (i != process_number){
            for (int j = 1; j<=number_of_messages; j++) {
                message m;
                m.ack = false;
                m.seq_number = j;
                m.proc_number = link.process_number;
                m.payload = "";
                //cout << "Send message " << m.seq_number << " to process " << m.proc_number << endl;
                link.send_to(i, m, sockfd);
            }
        }
    }

    vector<vector<bool>> messages_received(number_of_processes+1, vector<bool>(number_of_messages+1, false));
    int total_messages_received = 0;
<<<<<<< HEAD
    while(total_messages_received != (number_of_messages * (number_of_processes-1) )){
=======
    while(true){
>>>>>>> c093ffeb64cf1b0aed94dbc734313aeb1da0c6c9
        message m = link.pp2p_deliver();
    }

    cout << "Finally, we must have got " << total_messages_received << endl;

    while(1) {
		struct timespec sleep_time;
		sleep_time.tv_sec = 1;
		sleep_time.tv_nsec = 0;
		nanosleep(&sleep_time, NULL);
	}

	 */

}

void run_deliverer(Link* link, int number_of_processes){
    vector<int> delivered(number_of_processes, 0);
    while(true) {
        message msg = link->get_next_message();
        if (msg.ack) {
            // we received an ack;
            cout << "Received ack :) " << msg.proc_number << " " << msg.seq_number << endl;
            timer_killer_by_process_message[msg.proc_number][msg.seq_number].kill();
        } else {
            link->send_ack(msg.proc_number, msg.seq_number);
            if (delivered[msg.proc_number] < msg.seq_number) {
                link->pp2p_deliver(msg);
                delivered[msg.proc_number] = msg.seq_number;
            }
        }
    }
}
