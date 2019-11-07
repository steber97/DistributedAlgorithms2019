#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <iostream>

#include "Link.h"
#include "utilities.h"
#include "BebBroadcast.h"

using namespace std;

static int wait_for_start = 1;
bool initialization_phase = true;

int process_number;

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

    ofstream out("da_proc_" + to_string(process_number) + ".out");

    // Write log actions to output!
    mtx_log.lock();
    for (string line: log_actions){
        out << line << endl;
    }
    mtx_log.unlock();


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

    // it is a global variable, used even in the signal handler to manage the output file.
    process_number = atoi(argv[1]);

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

    struct sockaddr_in sock;
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
    BebBroadcast* beb_broadcast = new BebBroadcast(link, number_of_processes);

    //Broadcast* broadcast = new Broadcast(link, number_of_processes, number_of_messages);

    // Resize the number of acks (at the perfect link layer)
    acks.resize(number_of_processes+1, vector<bool>(number_of_messages+1, false));

    // Resize the delivered messages matrix (at the perfect link layer) It is used to avoid duplicates.
    pl_delivered.resize(number_of_processes+1, vector<bool>(number_of_messages+1, false));

    //wait until start signal
	while(wait_for_start) {
		struct timespec sleep_time;
		sleep_time.tv_sec = 0;
		sleep_time.tv_nsec = 1000;
		nanosleep(&sleep_time, NULL);
	}

    link->init();
	beb_broadcast->init();

    //broadcast messages
    printf("Broadcasting messages.\n");

    for (int i = 1; i <= number_of_messages; i++) {
        message msg(false, i, link->get_process_number(), "");
        broadcast_message broad_msg (i, link->get_process_number(), msg);
        beb_broadcast->beb_broadcast(broad_msg);
    }

    usleep(20000000);

    // Test Perfect Link
	// Try to send a lot of messages at the time.
//	for (int i = 1; i<=number_of_processes; i++){
//        if (i != process_number){
//            for (int j = 1; j<=number_of_messages; j++) {
//                cout << "Send message " << j << " from " << process_number << " to " << i << endl;
//                message m(false, j, link->get_process_number(), "");
//                link->send_to(i, m);
//            }
//        }
//    }

    while(1) {
		struct timespec sleep_time;
		sleep_time.tv_sec = 1;
		sleep_time.tv_nsec = 0;
		nanosleep(&sleep_time, NULL);
	}

}
