// Client side implementation of UDP client-server model 
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <iostream>
#include <thread>

#define PORT	 12001 
#define MAXLINE 1024 

using namespace std;


// Driver code 
int main(int argc, char** argv) { 
	int sockfd; 
	char buffer[MAXLINE]; 
	char hello[] = "Hello from client"; 
	struct sockaddr_in servaddr; 

	// Creating socket file descriptor 
	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
		perror("socket creation failed"); 
		exit(EXIT_FAILURE); 
	} 

	memset(&servaddr, 0, sizeof(servaddr)); 
	
	// Filling server information 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_port = htons(PORT); 
	inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr);
    //servaddr.sin_addr.s_addr = INADDR_ANY; 

	thread t1([&] {
        for (unsigned int i = 0; i < 100; i++){
			sendto(sockfd, (const char *)hello, strlen(hello), MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));  
			printf("t1 message sent\n");
		}
    });
	cout << "t1 started" << endl;

	thread t2([&] {
        for (unsigned int i = 0; i < 100; i++){
			sendto(sockfd, (const char *)hello, strlen(hello), MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));  
			printf("t2 message sent\n");
		}
    });
	cout << "t2 started" << endl;

	thread t3([&] {
        for (unsigned int i = 0; i < 100; i++){
			sendto(sockfd, (const char *)hello, strlen(hello), MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));  
			printf("t3 message sent\n");
		}
    });
	cout << "t3 started" << endl;

	thread t4([&] {
        for (unsigned int i = 0; i < 100; i++){
			sendto(sockfd, (const char *)hello, strlen(hello), MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));  
			printf("t4 message sent\n");
		}
    });	cout << "t4 started" << endl;

	thread t5([&] {
        for (unsigned int i = 0; i < 100; i++){
			sendto(sockfd, (const char *)hello, strlen(hello), MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));  
			printf("t5 message sent\n");
		}
    });	
	cout << "t5 started" << endl;

	thread t6([&] {
        for (unsigned int i = 0; i < 100; i++){
			sendto(sockfd, (const char *)hello, strlen(hello), MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));  
			printf("t6 message sent\n");
		}
    });
	cout << "t6 started" << endl;

	thread t7([&] {
        for (unsigned int i = 0; i < 100; i++){
			sendto(sockfd, (const char *)hello, strlen(hello), MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));  
			printf("t7 message sent\n");
		}
    });
	cout << "t7 started" << endl;

	thread t8([&] {
        for (unsigned int i = 0; i < 100; i++){
			sendto(sockfd, (const char *)hello, strlen(hello), MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));  
			printf("t8 message sent\n");
		}
    });
	cout << "t8 started" << endl;

	thread t9([&] {
        for (unsigned int i = 0; i < 100; i++){
			sendto(sockfd, (const char *)hello, strlen(hello), MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));  
			printf("t9 message sent\n");
		}
    });
	cout << "t9 started" << endl;

	thread t10([&] {
        for (unsigned int i = 0; i < 100; i++){
			sendto(sockfd, (const char *)hello, strlen(hello), MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));  
			printf("t10 message sent\n");
		}
    });
	cout << "t10 started" << endl;

	t1.join();
	t2.join();
	t3.join();
	t4.join();
	t5.join();
	t6.join();
	t7.join();
	t8.join();
	t9.join();
	t10.join();

	printf("All threads joined.\n");  

	close(sockfd); 
	return 0; 
} 
