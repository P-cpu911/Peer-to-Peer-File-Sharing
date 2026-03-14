#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, char* argv[]){
	//variables
	struct sockaddr_in sockaddr;
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(8784);
	sockaddr.sin_addr.s_addr = htons(INADDR_ANY);
	memset(sockaddr.sin_zero, 0, 8);

	//socket
	printf("starting socket...\n");
	int serSocket = socket(AF_INET, SOCK_STREAM, 0);
	if(serSocket == -1){
		perror("socket: Error making socket");
		exit(EXIT_FAILURE);
	}

	//bind
	printf("binding to port 8784...\n");
	int serBind = bind(serSocket, (struct sockaddr*) &sockaddr, sizeof(sockaddr));
	if(serBind == -1){
		perror("Bind: Bind failed");
		exit(EXIT_FAILURE);
	}

	//listen
	printf("listening...\n");
	int serListen = listen(serSocket, 3);
	if(serListen == -1){
		perror("listen: listen failed");
		exit(EXIT_FAILURE);
	}

	//accept
	struct sockaddr_in cliAddr;
	int addrlen = sizeof(cliAddr);
	int serAccept = accept(serSocket, (struct sockaddr*) &cliAddr, &addrlen);
	if(serAccept == -1){
		perror("Accept: Accept failed");
		exit(EXIT_FAILURE);
	}
	else{
		printf("Connection established!\n");
//		printf("Entering loop...\n");
//		int flag = 0;
//		int count = 0;
//		while(flag != 1){
//			printf("Enter 1 to exit!\n");
//			if(scanf("%d", &flag) != 1){
//				printf("You disappoint me!\n");
//				exit(EXIT_FAILURE);
//				//flag = 0;
//			}
//			if(flag != 1){
//				printf("Loop continues!\n");
//				count++;
//			}
//			else{
//				printf("Loop ended!\n");
//			}
//			if(count >= 10){
//			printf("Infi-loop detected. Terminate immediately!\n");
//				break;
//			}
//		}
	}

	//messaging
		int quit = 0;
		char msgBuff[1025];   //1024 +1 for delimiter \0
		while(quit == 0){
			read(serAccept, msgBuff, 1024);
			printf("Client> %s\n", msgBuff);
			printf("Server> ");
			fgets(msgBuff, 1024, stdin);	//will store \n from stdin into msgBuff
			write(serAccept, msgBuff, 1024);	//will write \n from msgBuff
			if(msgBuff[0] == '~'){
				quit = 1;
			}
		}

	return 0;
}