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

#define MSG_BUFFER 1025

// Simple arg parser
void argBreaker(char* input, char* cmd, char* arg){
    sscanf(input, "%s %s", cmd, arg);
}

// Connect function
int connecter(char* host, int PORT, char* message, char flag){
    struct hostent* hostNamePtr = gethostbyname(host);
    struct in_addr hostAddr;

    if(hostNamePtr == NULL){
        printf("gethostbyname failed!\n");
        return 1;
    }

    hostAddr.s_addr = *(in_addr_t*) hostNamePtr->h_addr_list[0];

    int cliSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(cliSocket == -1){
        perror("socket");
        return 1;
    }

    struct sockaddr_in serAddr;
    serAddr.sin_family = AF_INET;
    serAddr.sin_port = htons(PORT);
    serAddr.sin_addr = hostAddr;
    memset(&serAddr.sin_zero, 0, 8);

    if(connect(cliSocket, (struct sockaddr*)&serAddr, sizeof(serAddr)) < 0){
        perror("connect");
        return 1;
    }

    // Index server mode
    if(flag == 'i'){
        send(cliSocket, message, strlen(message), 0);

        char msgBuff[MSG_BUFFER];
        int byteRecv = recv(cliSocket, msgBuff, sizeof(msgBuff)-1, 0);

        if(byteRecv > 0){
            msgBuff[byteRecv] = '\0';
            printf("Index server> %s", msgBuff);
        }
    }

    // Peer sever mode(download)
    else if(flag == 'p'){
        send(cliSocket, message, strlen(message), 0);

        FILE* file = fopen(message, "wb");
        if(file == NULL){
            printf("File open error\n");
            close(cliSocket);
            return 1;
        }

        char buffer[MSG_BUFFER];
        int byteRecv;
        int total = 0;

        while((byteRecv = recv(cliSocket, buffer, sizeof(buffer), 0)) > 0){
            fwrite(buffer, 1, byteRecv, file);
            total += byteRecv;
        }

        fclose(file);
        printf("Downloaded %d bytes\n", total);
    }

    close(cliSocket);
    return 0;
}

// Main
int main(){
    char input[512];
    char command[64];
    char argument[256];

    while(1){
        printf(">>> ");

        if(fgets(input, sizeof(input), stdin) == NULL){
            break;
        }

        // Remove newline
        input[strcspn(input, "\n")] = '\0';

        argBreaker(input, command, argument);

        // Seed
        if(strcmp(command, "SEED") == 0){
            char msg[512];
            sprintf(msg, "SEED %s 127.0.0.1 9000\n", argument);

            connecter("localhost", 8784, msg, 'i');
        }

        // Get
        else if(strcmp(command, "SEARCH") == 0){
            char msg[512];
            sprintf(msg, "SEARCH %s\n", argument);

            connecter("localhost", 8784, msg, 'i');
        }

        // Get
        else if(strcmp(command, "GET") == 0){
            connecter("127.0.0.1", 9000, argument, 'p');
        }

        // Exit
        else if(strcmp(command, "EXIT") == 0){
            break;
        }

        else{
            printf("Invalid command\n");
        }
    }

    printf("Exit successfully!\n");
    return 0;
}
