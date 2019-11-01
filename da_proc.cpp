#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <thread>
#include <iostream>

#include "Manager.h"
#include "utilities.h"

using namespace std;

static int wait_for_start = 1;
bool initialization_phase = true;

static void start(int signum) {
    cout << signum << " received" << endl;
	wait_for_start = 0;

    initialization_phase = false;
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

	//set signal handlers
	signal(SIGUSR2, start);
	signal(SIGTERM, stop);
	signal(SIGINT, stop);


	//parse arguments, including membership
	//initialize application
	//start listening for incoming UDP packets
	printf("Initializing.\n");

	cout << "The process id is " << ::getpid() << endl;


	// Here we must initialize everything!
    Manager* manager = parse_input_data(argc, argv);




    //wait until start signal
	while(wait_for_start) {
		struct timespec sleep_time;
		sleep_time.tv_sec = 0;
		sleep_time.tv_nsec = 1000;
		nanosleep(&sleep_time, NULL);
	}


	//broadcast messages
	printf("Broadcasting messages.\n");


	//wait until stopped

    manager->run();

	while(1) {
		struct timespec sleep_time;
		sleep_time.tv_sec = 1;
		sleep_time.tv_nsec = 0;
		nanosleep(&sleep_time, NULL);
	}
}
