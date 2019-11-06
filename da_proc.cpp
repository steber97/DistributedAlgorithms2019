#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <thread>
#include <iostream>

#include "Link.h"
#include "utilities.h"
#include "TimerKiller.h"

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
    int total_number_of_processes = input_data.second->size();
    int total_number_of_messages = input_data.first;

    // Resize the number of acks
    acks.resize(total_number_of_processes+1, vector<bool>(total_number_of_messages+1, false));

	// the condition variable matrix has a conditional variable
    Link link(process_number, input_data.second);

    //wait until start signal
	while(wait_for_start) {
		struct timespec sleep_time;
		sleep_time.tv_sec = 0;
		sleep_time.tv_nsec = 1000;
		nanosleep(&sleep_time, NULL);
	}

	cout << "init" << endl;

	link.init();

	//broadcast messages
	printf("Broadcasting messages.\n");

	// Try to send a lot of messages at the time.
	for (int i = 1; i<=total_number_of_processes; i++){
        if (i != process_number){
            for (int j = 1; j<=total_number_of_messages; j++) {
                message m(false, j, link.process_number, "");
                cout << "Send message " << m.seq_number << " to process " << i << endl;
                link.send_to(i, m);
            }
        }
    }

    vector<vector<bool>> messages_acked(total_number_of_processes+1, vector<bool>(total_number_of_messages+1, false));
    int total_messages_acked = 0;
    while(true){
        message m = link.get_next_message();
        mtx_acks.lock();

        mtx_acks.unlock();
    }


    while(1) {
		struct timespec sleep_time;
		sleep_time.tv_sec = 1;
		sleep_time.tv_nsec = 0;
		nanosleep(&sleep_time, NULL);
	}

}
