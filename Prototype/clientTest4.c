#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>

#define PORT 8784
#define BUFFER_SIZE 1024

int main(int argc, char* argv[]) {
    struct hostent* hostNamePtr = NULL;
    struct in_addr hostAddr;

    if (argc > 1) {
        hostNamePtr = gethostbyname(argv[1]);
    } else {
        char hostName[1024];
        printf("Enter server name: ");
        fgets(hostName, 1024, stdin);
        hostName[strcspn(hostName, "\n")] = '\0';
        hostNamePtr = gethostbyname(hostName);
    }

    if (hostNamePtr == NULL) {
        printf("gethostbyname failed\n");
        return 1;
    }

    hostAddr.s_addr = *(in_addr_t*) hostNamePtr->h_addr_list[0];
    printf("Connecting to %s...\n", inet_ntoa(hostAddr));

    int cliSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (cliSocket == -1) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in serAddr;
    serAddr.sin_family = AF_INET;
    serAddr.sin_port = htons(PORT);
    serAddr.sin_addr = hostAddr;
    memset(serAddr.sin_zero, 0, 8);

    if (connect(cliSocket, (struct sockaddr*)&serAddr, sizeof(serAddr)) < 0) {
        perror("connect");
        return 1;
    }

    printf("Connected!\n");

    char msgBuff[BUFFER_SIZE];

    while (1) {
        memset(msgBuff, 0, BUFFER_SIZE);

        printf("Client> ");
        if (fgets(msgBuff, BUFFER_SIZE, stdin) == NULL)
            break;

        send(cliSocket, msgBuff, strlen(msgBuff), 0); // ✅ FIXED

        ssize_t byteRead = recv(cliSocket, msgBuff, BUFFER_SIZE - 1, 0);

        if (byteRead > 0) {
            msgBuff[byteRead] = '\0';
            printf("Server> %s\n", msgBuff);

            if (msgBuff[0] == '~')
                break;
        }
        else if (byteRead == 0) {
            printf("Server disconnected\n");
            break;
        }
        else {
            perror("recv");
            break;
        }
    }

    close(cliSocket);
    return 0;
}
