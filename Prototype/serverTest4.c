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
#define BUFFER_SIZE 1025

int main() {
    struct sockaddr_in sockaddr;
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(PORT);
    sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    memset(sockaddr.sin_zero, 0, 8);

    printf("Starting index server...\n");

    int serSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serSocket == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    int optVal = 1;
    setsockopt(serSocket, SOL_SOCKET, SO_REUSEADDR, &optVal, sizeof(int));

    // Nonblocking
    int fl = fcntl(serSocket, F_GETFL, 0);
    fcntl(serSocket, F_SETFL, fl | O_NONBLOCK);

    if (bind(serSocket, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(serSocket, MAX_CLIENT_IN_QUEUE) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    int clientFDs[MAX_CLIENT_ACTIVE];
    memset(clientFDs, 0, sizeof(clientFDs));

    printf("Index Server running on port %d...\n", PORT);

    while (1) {
        fd_set set;
        FD_ZERO(&set);
        FD_SET(serSocket, &set);

        int maxFD = serSocket;

        for (int i = 0; i < MAX_CLIENT_ACTIVE; i++) {
            if (clientFDs[i] > 0) {
                FD_SET(clientFDs[i], &set);
                if (clientFDs[i] > maxFD)
                    maxFD = clientFDs[i];
            }
        }

        select(maxFD + 1, &set, NULL, NULL, NULL);

        // Accept
        if (FD_ISSET(serSocket, &set)) {
            struct sockaddr_in cliAddr;
            socklen_t addrlen = sizeof(cliAddr);

            int newfd = accept(serSocket, (struct sockaddr*)&cliAddr, &addrlen);

            if (newfd >= 0) {
                printf("New connection: %d\n", newfd);

                // Nonblocking client
                int fl2 = fcntl(newfd, F_GETFL, 0);
                fcntl(newfd, F_SETFL, fl2 | O_NONBLOCK);

                for (int i = 0; i < MAX_CLIENT_ACTIVE; i++) {
                    if (clientFDs[i] == 0) {
                        clientFDs[i] = newfd;
                        break;
                    }
                }
            }
        }

        // Handle clients
        for (int i = 0; i < MAX_CLIENT_ACTIVE; i++) {
            int sd = clientFDs[i];

            if (sd > 0 && FD_ISSET(sd, &set)) {
                char buffer[BUFFER_SIZE];
                ssize_t valread = recv(sd, buffer, BUFFER_SIZE - 1, 0);

                if (valread > 0) {
                    buffer[valread] = '\0';
                    printf("[INDEX] Client %d requested: %s", sd, buffer);

                    // Response
                    char response[] = "127.0.0.1 9000\n";

                    send(sd, response, strlen(response), 0);

                    // Close after response (your client expects 1-shot)
                    close(sd);
                    clientFDs[i] = 0;
                }
                else if (valread == 0) {
                    printf("Client %d disconnected\n", sd);
                    close(sd);
                    clientFDs[i] = 0;
                }
                else {
                    if (errno != EAGAIN && errno != EWOULDBLOCK) {
                        perror("recv");
                        close(sd);
                        clientFDs[i] = 0;
                    }
                }
            }
        }
    }

    return 0;
}
