#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

int main(int argc, char* argv[]){
	//gethostbyname
	struct hostent* hostNamePtr = NULL;
	struct in_addr hostAddr;
	if(argc > 1){
 		hostNamePtr = gethostbyname(argv[1]);
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

	//connect
	struct sockaddr_in serAddr;
	serAddr.sin_family = AF_INET;
	serAddr.sin_port = htons(8784);
	serAddr.sin_addr = hostAddr;
	memset(&serAddr.sin_zero, 0, 8);
	int cliConnect = connect(cliSocket, (struct sockaddr*) &serAddr, sizeof(serAddr));
	if(cliConnect == -1){
		printf("Connect failed!\n");
		return 1;
	}

	else{
                printf("Connection established!\n");
//                printf("Entering loop...\n");
//                int flag = 0;
//                while(flag != 1){
//                        printf("Enter 1 to exit!\n");
//                        if(scanf("%d", &flag) != 1){
//                                printf("You disappoint me!\n");
//                                exit(EXIT_FAILURE);
//                                //flag = 0;
//                        }
//                        if(flag != 1){
//                                printf("Loop continues!\n");
//                        }
//                        else{
//                                printf("Loop ended!\n");
//                        }
//                }
        }

	//messaging
	while (1) {
		char msg[1024];

		// ---- RECEIVE ----
		ssize_t bytes_received = recv(cliSocket, msg, sizeof(msg) - 1, 0);
		if (bytes_received <= 0) {
			printf("Server disconnected or recv error.\n");
			break;
		}

		msg[bytes_received] = '\0';
		printf("Server says: %s\n", msg);

		// ---- SEND ----
		printf("Client, enter your message...\n");
		if (fgets(msg, sizeof(msg), stdin) == NULL) {
			printf("Input error!\n");
			break;
		}

		size_t len = strlen(msg);
		if (send(cliSocket, msg, len, 0) == -1) {
			perror("send failed");
			break;
		}

		if (msg[0] == '~') {
			printf("Client exiting chat.\n");
			break;
		}
	}


	//close socket
    close(cliSocket);

	return 0;
}
