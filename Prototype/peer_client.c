#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>

//#define PORT 8784
#define MSG_BUFFER 1025	//+1 for \0, in case necessary

int connecter(int argCount, char* argVect[5], unsigned int PORT, char flag){
	//gethostbyname
	struct hostent* hostNamePtr = NULL;
	struct in_addr hostAddr;
	if(argCount > 1){
 		hostNamePtr = gethostbyname(argVect[1]);
	}
	else{
		char hostName[1024];
		printf("Please input server name...\n");
		if(fgets(hostName, 1024, stdin) == NULL){
			printf("Input error!\n");
			return 1;
		}
		hostName[strcspn(hostName, "\n")] = '\0';
		hostNamePtr = gethostbyname(hostName);
	}
	if(hostNamePtr == NULL){
                printf("get host by name failed!\n");
		return 1;
	}
	else{
		//simply put: This is pointer casting. All pointers have the same size, so one pointer can be treated to another type. The problem is you can't just cast any pointers to another type and expect it to work like normal. But at least the compiler will not complain in this case, as the structs are pretty similar. So basically, with that in mind, the string is casted into an uint_32 (unsigned integer 32 bits -> 4 bytes, which is the normal size of a normal integer), then it is dereference back to value mode and assigned to s_addr.
		hostAddr.s_addr = *(in_addr_t*) hostNamePtr->h_addr_list[0];	//Probably memcpy() is better
		printf("Establishing connection to %s ...\n", inet_ntoa(hostAddr));
	}

	//socket
	int cliSocket = socket(AF_INET, SOCK_STREAM, 0);
	if(cliSocket == -1){
		printf("Socket init failed!\n");
		return 1;
	}

	//socket option
        int optVal = 1;
        int sockOption = setsockopt(cliSocket, SOL_SOCKET, SO_REUSEADDR, &optVal, sizeof(int));
        if(sockOption == -1){
                perror("setsockopt()");
                return 1;
        }

	//Nonblocking socket fd -> Nonblocking socket()
//        int fl = fcntl(cliSocket, F_GETFL, 0);
//        fl |= O_NONBLOCK;
//        fcntl(cliSocket, F_SETFL, fl);

	//connect
	struct sockaddr_in serAddr;
	serAddr.sin_family = AF_INET;
	serAddr.sin_port = htons(PORT);
	serAddr.sin_addr = hostAddr;
	memset(&serAddr.sin_zero, 0, 8);
	int cliConnect = connect(cliSocket, (struct sockaddr*) &serAddr, sizeof(serAddr));
	if(cliConnect == -1){
		if(errno == EINPROGRESS){
			//Nothing...yet
		}
		else{
			perror("connect()");
			return 1;
		}
	}

	else{
                printf("Connection established!\n");

        }

		//messaging
		if(flag == 'i'){
			char msgBuff[MSG_BUFFER];   //1024 +1 for delimiter \0;
//			while(1){
				//ArgVect3NewLinePosition
				int av3nlp = strcspn(argVect[3], "\n");
				argVect[3][av3nlp] = '\n';
				argVect[3][av3nlp+1] = '\0';
				send(cliSocket, argVect[3], strlen(argVect[3]), 0);

				printf("Index server> ");
				int cursor = 0;
				int byteRecv = 0;
				int complete = 0;
				
				while (complete == 0) {
					byteRecv = recv(cliSocket, msgBuff + cursor, sizeof(msgBuff) - 1 - cursor, 0);
					if (byteRecv <= 0){
						// handle disconnect or error 
						break;
					}

					char* newlinePtr = strchr(msgBuff, '\n');
					if (newlinePtr != NULL) {
						*newlinePtr = '\0';
						printf("%s\n", msgBuff);

						cursor = (msgBuff + byteRecv) - (newlinePtr + 1);

						if (cursor == 0) {
							complete = 1;
							continue;
						} 
						else {
							memmove(msgBuff, newlinePtr + 1, cursor);
							msgBuff[cursor] = '\0';
						}
					} 
					else {
						if (cursor + byteRecv < sizeof(msgBuff) - 1) {
							cursor += byteRecv;
						} 
						else {
							msgBuff[cursor] = '\0';
							printf("%s", msgBuff);
							cursor = 0;
						}
					}
				}



//				printf("Server> %s\n", msgBuff);
//				if(msgBuff[0] == '~'){
//					break;
//				}
//			}
		}
		else if(flag == 'p'){
			send(cliSocket, argVect[3], strlen(argVect[3]), 0);

			int n;
			int total = 0;
			int filled = 0;
			int byteRecv = 0;
			char msgBuff[MSG_BUFFER]; // frame buffer
			FILE* file = fopen(argVect[3], "ab+");

			while ((byteRecv = recv(cliSocket, msgBuff + filled, (MSG_BUFFER-1) - filled, 0)) > 0) {
				filled += n;

				// If frame is full, write it out
				if (filled == MSG_BUFFER-1) {
					fwrite(msgBuff, 1, MSG_BUFFER-1, file);
					total += MSG_BUFFER-1;
					filled = 0;
				}
			}

			// At EOF: if there's a partial frame, write it
			if (filled > 0) {
				fwrite(msgBuff, 1, filled, file);
				total += filled;
			}

		}
		else{
			printf("Invalid flag\n");
			return 1;
		}
	close(cliSocket);
	return 0;
}


void argBreaker(char** argVect){
	int index = 2;
	int start = 0;
	int len = strlen(argVect[0]);
	for(int i = 0; i < len-1; i++){
		if(argVect[0][i] == ' '){
			argVect[0][i] = '\0';
			memmove(argVect[index], argVect[0] + start, (i-start)+1);
			index++;
			start = i+1;
		}
	}
	argVect[0][len-1] = '\0';
	memmove(argVect[index], argVect[0] + start, len - start);
}

int main(){
	//For example: SEARCH hello.txt
	//argVect[0] = SEARCH hello.txt, argVect[1] = localhost (IP/hostname), argVect[2] = SEARCH, argVect[3] = hello.txt
	char* argVect[5];
	char arg0[512];
	char arg1[85];
	char arg2[85];
	char arg3[256];
	char arg4[85];

	argVect[0] = arg0;
	//argVect[1] = arg1;
	argVect[2] = arg2;
	argVect[3] = arg3;
	argVect[4] = arg4;

	//char arg1[] = "localhost";
	//argVect[1] = arg1;

	while(1){
		printf(">>> ");
		if(fgets(argVect[0], 512, stdin) == NULL){
			printf("Sry guys, it was me\n");
			break;
		}
		argBreaker(argVect);
		if(strcmp(argVect[2], "SEARCH") == 0){
			char arg1[] = "localhost";	//Replace this with Index server IP
			argVect[1] = arg1;
			int PORT = 8784;	//Replace this with Index server Port
			connecter(5, argVect, PORT, 'i');
		}
		else if(strcmp(argVect[2], "SEED") == 0){
			//Downloading case
		}
		else if(strcmp(argVect[2], "EXIT") == 0){
			break;
		}
		else{
			printf("Invalid command\n");
		}
	}

	printf("Exit successfully!\n");
	return 0;
}