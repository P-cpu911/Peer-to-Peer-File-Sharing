#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define INDEX_SERVER_PORT 8784
#define MY_PORT 9000 // Change this for different peers (9001, 9002, etc.)

// Server Thread: Listens for other peers wanting to download
void *start_seeding(void *arg) {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(MY_PORT);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 5);

    while(1) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        printf("\n\033[0;35m[THREAD_02] Incoming download request! Sending data...\033[0m\n");
        send(new_socket, "FILE_DATA_STREAM_SUCCESS", 24, 0);
        close(new_socket);
    }
}

int main() {
    pthread_t tid;
    pthread_create(&tid, NULL, start_seeding, NULL);

    printf("\033[0;36m[root@RHEL10 ~]# P2P NODE INITIALIZED\n");
    printf("PORT: %d | STATUS: SEEDING_ACTIVE\033[0m\n\n", MY_PORT);

    char command[100], filename[50];
    
    while(1) {
        printf("Enter command (SEED <filename> / exit): ");
        scanf("%s", command);

        if (strcmp(command, "SEED") == 0) {
            scanf("%s", filename);
            int sock = socket(AF_INET, SOCK_STREAM, 0);
            struct sockaddr_in serv_addr;
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = htons(INDEX_SERVER_PORT);
            inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

            if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == 0) {
                char msg[100];
                sprintf(msg, "SEED %s %d", filename, MY_PORT);
                send(sock, msg, strlen(msg), 0);
                printf("\033[0;32m[SUCCESS] File registered on Index Server.\033[0m\n");
            }
            close(sock);
        } else if (strcmp(command, "exit") == 0) {
            printf("\033[0;31m[root@RHEL10 ~]# exit\nLogout... System Offline.\033[0m\n");
            exit(0);
        }
    }
    return 0;
}
