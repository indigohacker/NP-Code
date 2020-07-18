// Write CPP code here 
#include <netdb.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <time.h>
#define MAX 80 
#define PORT 8080 
#define SA struct sockaddr 
void func(int sockfd) 
{ 
	char buff[MAX]; 
    clock_t tim;
	int n, ctr=0, throughput=0; 
    float delay=0;
	for (;;) { 
		bzero(buff, sizeof(buff)); 
		printf("Enter the string : "); 
		n = 0; 
		while ((buff[n++] = getchar()) != '\n'); 
        tim=clock();
		write(sockfd, buff, sizeof(buff)); 
        tim=clock()-tim;
        char result[10];
        sprintf(result, "%f", (float)tim/CLOCKS_PER_SEC);
        write(sockfd, result, sizeof(result));
        // if msg contains "Exit" then server exit and chat ended. 
		if (strncmp("exit", buff, 4) == 0) { 
			printf("%d is average throughput, %f ms is average delay, Client Exit...\n", throughput, delay);
			break; 
		}
		bzero(buff, sizeof(buff)); 
        int temp=read(sockfd, buff, sizeof(buff)); 
        throughput=((throughput*ctr)+temp)/(ctr+1);
        read(sockfd, result, sizeof(result));
        float temps=strtof(result, NULL);
        delay=((delay*ctr)+temps)/(ctr+1);
        ctr++;

		printf("\tFrom Server : %s", buff); 
		if ((strncmp(buff, "exit", 4)) == 0) { 
			printf("%d is average throughput, %f ms is average delay, Client Exit...\n", throughput, delay); 
			break; 
		} 
	} 
} 

int main() 
{ 
	int sockfd, connfd; 
	struct sockaddr_in servaddr, cli; 

	// socket create and varification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) { 
		printf("socket creation failed...\n"); 
		exit(0); 
	} 
	else
		printf("Socket successfully created..\n"); 
	bzero(&servaddr, sizeof(servaddr)); 

	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1"); 
	servaddr.sin_port = htons(PORT); 

	// connect the client socket to server socket 
	if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) { 
		printf("connection with the server failed...\n"); 
		exit(0); 
	} 
	else
		printf("connected to the server..\n"); 

	// function for chat 
	func(sockfd); 

	// close the socket 
	close(sockfd); 
} 
