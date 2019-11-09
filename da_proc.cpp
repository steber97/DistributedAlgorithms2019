#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <iostream>

#include "Link.h"
#include "utilities.h"
#include "UrBroadcast.h"
#include "BeBroadcast.h"
#include "FifoBroadcast.h"

using namespace std;

static int wait_for_start = 1;

int process_number;

int sockfd;

static void start(int signum) {
	wait_for_start = 0;
}


static void stop(int signum) {
	//reset signal handlers to default
	signal(SIGTERM, SIG_DFL);
	signal(SIGINT, SIG_DFL);

	// Stop delivering and sending message at the pp2p layer!
    close(sockfd);

    mtx_pp2p_sender.lock();
    stop_pp2p_sender = true;
    mtx_pp2p_sender.unlock();

    mtx_pp2p_receiver.lock();
    stop_pp2p_receiver = true;
    mtx_pp2p_receiver.unlock();

    mtx_pp2p_get_msg.lock();
    stop_pp2p_get_msg = true;
    mtx_pp2p_get_msg.unlock();

    sleep(2);   // wait for sender and receiver to stop, so that after the below writing no message is received or sent.

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
    // Give time to every detached thread
    sleep(5);

	//exit directly from signal handler
	terminate();
}


int main(int argc, char** argv) {

    if (argc != 4){
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
	unordered_map<int, pair<string, int>>* input_data = parse_input_data(membership_file);
    int number_of_processes = input_data->size();
    int number_of_messages = stoi(argv[3]);

    struct sockaddr_in sock;

    string ip_address = (*input_data)[process_number].first;
    int port = (*input_data)[process_number].second;
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

    Link* link = new Link(sockfd, process_number, input_data);
    BeBroadcast* beb = new BeBroadcast(link, number_of_processes, number_of_messages);
    UrBroadcast* urb = new UrBroadcast(beb, number_of_processes, number_of_messages);
    FifoBroadcast* fb = new FifoBroadcast(urb, number_of_processes);

    //Broadcast* broadcast = new Broadcast(link, number_of_processes, number_of_messages);

    // Resize the number of acks (at the perfect link layer)
    acks.resize(number_of_processes+1, unordered_set<long long>());

    // Resize the delivered messages matrix (at the perfect link layer) It is used to avoid duplicates.
    pl_delivered.resize(number_of_processes+1, unordered_set<long long>());

    link->init();
    beb->init();
    urb->init();
    fb->init();

    //wait until start signal
	while(wait_for_start) {
		struct timespec sleep_time;
		sleep_time.tv_sec = 0;
		sleep_time.tv_nsec = 1000;
		nanosleep(&sleep_time, NULL);
	}

    //broadcast messages
    printf("Broadcasting messages.\n");

    for (int i = 1; i <= number_of_messages; i++) {
        b_message msg (i, link->get_process_number());
        //beb->beb_broadcast(msg);
        //urb->urb_broadcast(msg);
        fb->fb_broadcast(msg);
    }
//
//    // Try to send a lot of messages at the time.
//    for (int i = 1; i<=number_of_processes; i++){
//        b_message msg (i, link->get_process_number());
//        if (i != process_number){
//            for (int j = 1; j<=number_of_messages; j++) {
//                pp2p_message m(false, j, link->get_process_number(), msg);
//                link->send_to(i, m);
//            }
//        }
//    }
//
//    while(true){
//        pp2p_message m = link->get_next_message();
//    }
//
    while(1) {
		struct timespec sleep_time;
		sleep_time.tv_sec = 1;
		sleep_time.tv_nsec = 0;
		nanosleep(&sleep_time, NULL);
	}

}
