#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/select.h>

#define PORT 8784
#define MAX_CLIENT_IN_QUEUE 20
#define MAX_CLIENT_ACTIVE 10
#define BUFFER_SIZE 1025	//Last position for /0...maybe

int main(int argc, char* argv[]){
	//variables
	struct sockaddr_in sockaddr;
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(PORT);
	sockaddr.sin_addr.s_addr = htons(INADDR_ANY);
	memset(sockaddr.sin_zero, 0, 8);

	//socket
	printf("starting socket...\n");
	int serSocket = socket(AF_INET, SOCK_STREAM, 0);
	if(serSocket == -1){
		perror("socket: Error making socket\n");
		exit(EXIT_FAILURE);
	}

	//socket option
	int optVal = 1;
	int sockOption = setsockopt(serSocket, SOL_SOCKET, SO_REUSEADDR, &optVal, sizeof(int));
	if(sockOption == -1){
		perror("setsockopt: Error\n");
		exit(EXIT_FAILURE);
	}

	//Nonblocking socket fd -> Nonblocking accept()
	int fl = fcntl(serSocket, F_GETFL, 0);
	fl |= O_NONBLOCK;
	fcntl(serSocket, F_SETFL, fl);

	//bind
	printf("binding to port %d...\n", PORT);
	int serBind = bind(serSocket, (struct sockaddr*) &sockaddr, sizeof(sockaddr));
	if(serBind == -1){
		perror("Bind: Bind failed\n");
		exit(EXIT_FAILURE);
	}

	//listen
	printf("listening...\n");
	int serListen = listen(serSocket, MAX_CLIENT_IN_QUEUE);
	if(serListen == -1){
		perror("listen: listen failed\n");
		exit(EXIT_FAILURE);
	}

	//accept
	//Client FDs' array
	int clientFDs[MAX_CLIENT_ACTIVE];
	memset(clientFDs, 0, sizeof(clientFDs));
	//Updated to a while-loop to auto accept()
	while(1){
		//fd_set set (reset every session)
		fd_set set;
		FD_ZERO(&set);
		FD_SET(serSocket, &set);
		//We assume serSocket is max (as in fd value). This will serve select() later
		int maxFD = serSocket;
		//put already connected (accept()-ed) back into the set
		for(int i = 0; i < MAX_CLIENT_ACTIVE; i++){
			if(clientFDs[i] > 0){
				FD_SET(clientFDs[i], &set);
			}
			//Adjust maxFD
			if(clientFDs[i] > maxFD){
				maxFD = clientFDs[i];
			}
		}
		//select()
		//No timeout, no sets for sending and error, only receiving. Might change later
		select(maxFD + 1, &set, NULL, NULL, NULL);

		//accept() (wake accept() if there is a request to)
		if(FD_ISSET(serSocket, &set)){
			struct sockaddr_in cliAddr;
			int addrlen = sizeof(cliAddr);
			int serAccept = accept(serSocket, (struct sockaddr*) &cliAddr, &addrlen);
			if(serAccept == -1){
				if(errno == EAGAIN || errno == EWOULDBLOCK){
					continue;	//for now
				}
				else{
					perror("accept()");
					exit(EXIT_FAILURE);
				}
			}
			else{
				printf("Connection established!\n");
				//Nonblocking
				int fl2 = fcntl(serAccept, F_GETFL, 0);
				fl2 |= O_NONBLOCK;
				fcntl(serAccept, F_SETFL, fl2);
				//Add to multiplexing set
				int skip = 0;
				for(int i = 0; i < MAX_CLIENT_ACTIVE; i++){
					if(clientFDs[i] == 0){
						clientFDs[i] = serAccept;
						skip = 1;
						break;
					}
				}
				if(skip == 1){
					continue;
				}
			}
		}

		//messaging
		//Loop through client list and reply one by one
		char msgBuff[BUFFER_SIZE];
		for(int i = 0; i < MAX_CLIENT_ACTIVE; i++) {
    			int sd = clientFDs[i];

    			// ONLY read if select() flagged this specific socket
    			if (sd > 0 && FD_ISSET(sd, &set)) {
        			ssize_t valread = read(sd, msgBuff, BUFFER_SIZE - 1);
        			if (valread > 0) {
            				msgBuff[valread] = '\0';
            				printf("Client %d> %s\n", sd, msgBuff);
					//Write data here to send
					char input[1024];
					printf("Server> ");
					fgets(input, 1000, stdin);
//            				send(sd, "ACK\n", 4, 0);
					send(sd, input, sizeof(input), 0);
					printf("\n");
        			}
        			else if (valread == 0) {
            				// Truly disconnected
            				printf("Client %d has disconnected!\n", sd);
            				close(sd);
            				clientFDs[i] = 0;
        			}
        			else {
            				// Handle actual errors, but skip "would block" signals
            				if (errno != EAGAIN && errno != EWOULDBLOCK) {
                			perror("read error");
                			close(sd);
                			clientFDs[i] = 0;
            			}
        		}
    		}
	}
}


	return 0;
}
