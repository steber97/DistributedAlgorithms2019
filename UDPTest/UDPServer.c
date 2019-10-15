
// Server side implementation of UDP client-server model 
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 

#define PORT    12001 
#define MAXLINE 1024 

// Driver code 
int main() {  
	int sockfd; 
	char buffer[MAXLINE]; 
	char hello[] = "Hello from server"; 
	struct sockaddr_in servaddr, cliaddr; 
	
	// Creating socket file descriptor 
	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
		perror("socket creation failed"); 
		exit(EXIT_FAILURE); 
	} 
	
	memset(&servaddr, 0, sizeof(servaddr)); 
	memset(&cliaddr, 0, sizeof(cliaddr)); 
	
	// Filling server information 
	servaddr.sin_family = AF_INET; // IPv4 
    inet_pton(AF_INET, "127.0.0.1", &(servaddr.sin_addr));
	// servaddr.sin_addr = INADDR_ANY; 
	servaddr.sin_port = htons(PORT); 
	
	// Bind the socket with the server address 
	if ( bind(sockfd, (const struct sockaddr *)&servaddr, 
			sizeof(servaddr)) < 0 ) { 
		perror("bind failed"); 
		exit(EXIT_FAILURE); 
	} 
	
	unsigned int len;
    int n; 
	long int counter = 0;
    while(true){
		// This timeout is used to stop recvfrom after 1 second if nothing interesting happens.
		struct timeval timeout = {1, 0}; 
		printf("Waiting for something to happen %ld\n", counter ++);
		static volatile bool _threadGoAway = false;	

		fd_set readSet;
		FD_ZERO(&readSet);
		FD_SET(sockfd, &readSet);

		if (select(sockfd+1, &readSet, NULL, NULL, &timeout) >= 0)
		{
			printf("select \n");
			if (_threadGoAway)
			{
				printf("MyThread:  main thread wants me to scram, bye bye!\n");
				
			}
			else if (FD_ISSET(sockfd, &readSet))
			{
				printf("Ciao\n");
				char buf[1024];
				n = recvfrom(sockfd, (char *)buffer, MAXLINE, MSG_WAITALL, ( struct sockaddr *) &cliaddr, &len); 
				buffer[n] = '\0'; 
				printf("Client : %s\n", buffer); 
				printf("Client has port %u\n", cliaddr.sin_port);
				sendto(sockfd, (const char *)hello, strlen(hello), 
					MSG_CONFIRM, (const struct sockaddr *) &cliaddr, 
						len); 
				printf("Hello message sent.\n"); 
			}
		}
      	else{
			printf("select\n");
		}
         
    }
	return 0; 
} 
