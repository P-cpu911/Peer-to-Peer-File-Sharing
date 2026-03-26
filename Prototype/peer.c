#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define INDEX_PORT 8784
int MY_P2P_PORT; // Port for other peers to connect to

// Background thread: Listen for other peers to download from YOU
void *seeder_thread(void *arg) {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(MY_P2P_PORT);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 5);

    while(1) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        char content[] = "This is the content of the file you requested.";
        send(new_socket, content, strlen(content), 0);
        close(new_socket);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) { printf("Usage: ./peer <port>\n"); return 1; }
    MY_P2P_PORT = atoi(argv[1]);

    pthread_t tid;
    pthread_create(&tid, NULL, seeder_thread, NULL);

    printf("\033[0;34m[root@RHEL10 ~]# P2P NODE INITIALIZED\n");
    printf("PORT: %d | STATUS: SEEDING_ACTIVE\033[0m\n\n", MY_P2P_PORT);

    char cmd[20], file[50], buffer[1024];

    while(1) {
        printf("Enter command (SEED <filename> / SEARCH <filename> / GET <filename> / exit): ");
        scanf("%s", cmd);

        if (strcmp(cmd, "SEED") == 0) {
            scanf("%s", file);
            int s = socket(AF_INET, SOCK_STREAM, 0);
            struct sockaddr_in addr = {AF_INET, htons(INDEX_PORT)};
            inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
            if(connect(s, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
                sprintf(buffer, "SEED %s %d", file, MY_P2P_PORT);
                send(s, buffer, strlen(buffer), 0);
                printf("\033[0;32m[SUCCESS] File registered on Index Server.\033[0m\n");
            }
            close(s);
        } 
        else if (strcmp(cmd, "SEARCH") == 0) {
            scanf("%s", file);
            int s = socket(AF_INET, SOCK_STREAM, 0);
            struct sockaddr_in addr = {AF_INET, htons(INDEX_PORT)};
            inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
            connect(s, (struct sockaddr *)&addr, sizeof(addr));
            sprintf(buffer, "SEARCH %s", file);
            send(s, buffer, strlen(buffer), 0);
            memset(buffer, 0, 1024);
            recv(s, buffer, 1024, 0);
            printf("\033[0;33m[INDEX_RESULT] %s\033[0m\n", buffer);
            close(s);
        }
        else if (strcmp(cmd, "GET") == 0) {
            scanf("%s", file);
            char target_ip[20]; int target_port;
            printf("Enter Peer IP and Port from SEARCH result: ");
            scanf("%s %d", target_ip, &target_port);

            int s = socket(AF_INET, SOCK_STREAM, 0);
            struct sockaddr_in addr = {AF_INET, htons(target_port)};
            inet_pton(AF_INET, target_ip, &addr.sin_addr);
            if(connect(s, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
                memset(buffer, 0, 1024);
                recv(s, buffer, 1024, 0);
                printf("\033[1;32m[DOWNLOAD_COMPLETE] Content: %s\033[0m\n", buffer);
            }
            close(s);
        }
        else if (strcmp(cmd, "exit") == 0) { printf("Shutting down...\n"); exit(0); }
    }
    return 0;
}
