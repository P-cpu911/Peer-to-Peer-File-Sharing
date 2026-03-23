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

int connecter(int argCount, char* argVect[5], unsigned int PORT, char flag){
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
        printf("gethostbyname failed!\n");
        return 1;
    }

    hostAddr.s_addr = *(in_addr_t*) hostNamePtr->h_addr_list[0];
    printf("Connecting to %s...\n", inet_ntoa(hostAddr));

    int cliSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(cliSocket == -1){
        perror("socket");
        return 1;
    }

    int optVal = 1;
    setsockopt(cliSocket, SOL_SOCKET, SO_REUSEADDR, &optVal, sizeof(int));

    struct sockaddr_in serAddr;
    serAddr.sin_family = AF_INET;
    serAddr.sin_port = htons(PORT);
    serAddr.sin_addr = hostAddr;
    memset(serAddr.sin_zero, 0, 8);

    if(connect(cliSocket, (struct sockaddr*) &serAddr, sizeof(serAddr)) < 0){
        perror("connect");
        close(cliSocket);
        return 1;
    }

    printf("Connection established!\n");

    // Index sever mode
    if(flag == 'i'){
        char msgBuff[MSG_BUFFER];
        memset(msgBuff, 0, MSG_BUFFER);

        int av3nlp = strcspn(argVect[3], "\n");
        argVect[3][av3nlp] = '\n';
        argVect[3][av3nlp+1] = '\0';

        send(cliSocket, argVect[3], strlen(argVect[3]), 0);

        printf("Index server> ");

        int cursor = 0;
        int byteRecv = 0;
        int complete = 0;

        while (!complete) {
            byteRecv = recv(cliSocket, msgBuff + cursor, MSG_BUFFER - 1 - cursor, 0);

            if (byteRecv <= 0){
                printf("Connection closed or error\n");
                break;
            }

            msgBuff[cursor + byteRecv] = '\0';

            char* newlinePtr = strchr(msgBuff, '\n');

            if (newlinePtr != NULL) {
                *newlinePtr = '\0';
                printf("%s\n", msgBuff);

                int remaining = (msgBuff + cursor + byteRecv) - (newlinePtr + 1);

                if (remaining > 0) {
                    memmove(msgBuff, newlinePtr + 1, remaining);
                    cursor = remaining;
                } else {
                    complete = 1;
                }
            } else {
                cursor += byteRecv;

                if (cursor >= MSG_BUFFER - 1) {
                    printf("%s", msgBuff);
                    cursor = 0;
                    memset(msgBuff, 0, MSG_BUFFER);
                }
            }
        }
    }

    // Peer sever mode
    else if(flag == 'p'){
        send(cliSocket, argVect[3], strlen(argVect[3]), 0);

        int total = 0;
        int filled = 0;
        int byteRecv = 0;
        char msgBuff[MSG_BUFFER];

        FILE* file = fopen(argVect[3], "wb");
        if (!file) {
            perror("fopen");
            close(cliSocket);
            return 1;
        }

        while ((byteRecv = recv(cliSocket, msgBuff + filled, (MSG_BUFFER-1) - filled, 0)) > 0) {

            filled += byteRecv;

            if (filled == MSG_BUFFER-1) {
                fwrite(msgBuff, 1, filled, file);
                total += filled;
                filled = 0;
            }
        }

        if (filled > 0) {
            fwrite(msgBuff, 1, filled, file);
            total += filled;
        }

        printf("Downloaded %d bytes\n", total);

        fclose(file);
    }

    else{
        printf("Invalid flag\n");
        close(cliSocket);
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
    char* argVect[5];
    char arg0[512];
    char arg2[85];
    char arg3[256];
    char arg4[85];

    argVect[0] = arg0;
    argVect[2] = arg2;
    argVect[3] = arg3;
    argVect[4] = arg4;

    while(1){
        printf(">>> ");
        if(fgets(argVect[0], 512, stdin) == NULL){
            printf("Exit\n");
            break;
        }

        argBreaker(argVect);

        if(strcmp(argVect[2], "SEARCH") == 0){
            char arg1[] = "localhost";
            argVect[1] = arg1;
            connecter(5, argVect, 8784, 'i');
        }
        else if(strcmp(argVect[2], "GET") == 0){
            char arg1[] = "localhost";
            argVect[1] = arg1;
            connecter(5, argVect, 9000, 'p');
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
