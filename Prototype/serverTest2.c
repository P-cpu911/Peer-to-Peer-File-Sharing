#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8784
#define BUFFER_SIZE 1024

int main() {
    struct sockaddr_in sockaddr;
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(PORT);
    sockaddr.sin_addr.s_addr = htonl(INADDR_ANY); // ✅ FIXED
    memset(sockaddr.sin_zero, 0, 8);

    printf("Starting socket...\n");

    int serSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serSocket == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    printf("Binding to port %d...\n", PORT);
    if (bind(serSocket, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    printf("Listening...\n");
    if (listen(serSocket, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in cliAddr;
    socklen_t addrlen = sizeof(cliAddr);

    int serAccept = accept(serSocket, (struct sockaddr*)&cliAddr, &addrlen);
    if (serAccept < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    printf("Connection established!\n");

    char msgBuff[BUFFER_SIZE];

    while (1) {
        memset(msgBuff, 0, BUFFER_SIZE);

        ssize_t bytesRead = read(serAccept, msgBuff, BUFFER_SIZE - 1);

        if (bytesRead > 0) {
            msgBuff[bytesRead] = '\0';
            printf("Client> %s\n", msgBuff);

            printf("Server> ");
            fgets(msgBuff, BUFFER_SIZE, stdin);

            write(serAccept, msgBuff, strlen(msgBuff));

            if (msgBuff[0] == '~') {
                break;
            }
        }
        else if (bytesRead == 0) {
            printf("Client disconnected\n");
            break;
        }
        else {
            perror("read");
            break;
        }
    }

    close(serAccept);
    close(serSocket);

    return 0;
}
