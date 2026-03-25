#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define INDEX_PORT 8784
int MY_P2P_PORT;
char SERVER_IP[20];

// The background thread that waits for peers to download from YOU
void *seeder_thread(void *arg) {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(MY_P2P_PORT);
    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 5);
    while(1) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        char content[] = "SUCCESS: P2P Data Stream received from Remote Peer!";
        send(new_socket, content, strlen(content), 0);
        close(new_socket);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) { printf("Usage: ./peer <your_port> <server_ip>\n"); return 1; }
    MY_P2P_PORT = atoi(argv[1]);
    strcpy(SERVER_IP, argv[2]);

    pthread_t tid;
    pthread_create(&tid, NULL, seeder_thread, NULL);
    printf("\033[0;34m[NODE] Online on Port %d. Connected to Server: %s\033[0m\n", MY_P2P_PORT, SERVER_IP);

    char cmd[20], file[50], buffer[1024];
    while(1) {
        printf("\nCommands: SEED <file> | SEARCH <file> | GET <file> | exit\n> ");
        if (scanf("%s", cmd) == EOF) break;
        if (strcmp(cmd, "exit") == 0) exit(0);
        scanf("%s", file);

        if (strcmp(cmd, "SEED") == 0 || strcmp(cmd, "SEARCH") == 0) {
            int s = socket(AF_INET, SOCK_STREAM, 0);
            struct sockaddr_in addr = {AF_INET, htons(INDEX_PORT)};
            inet_pton(AF_INET, SERVER_IP, &addr.sin_addr);
            if(connect(s, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
                sprintf(buffer, "%s %s %d", cmd, file, MY_P2P_PORT);
                send(s, buffer, strlen(buffer), 0);
                if(strcmp(cmd, "SEARCH") == 0) {
                    memset(buffer, 0, 1024);
                    recv(s, buffer, 1024, 0);
                    printf("\033[0;33m[INDEX_SEARCH_RESULT] %s\033[0m\n", buffer);
                } else printf("\033[0;32m[OK] Registered %s to Index Server.\033[0m\n", file);
            } else printf("\033[0;31m[ERR] Server Unreachable at %s\033[0m\n", SERVER_IP);
            close(s);
        }
        else if (strcmp(cmd, "GET") == 0) {
            char t_ip[20]; int t_port;
            printf("Enter Peer IP and Port from SEARCH: ");
            scanf("%s %d", t_ip, &t_port);
            int s = socket(AF_INET, SOCK_STREAM, 0);
            struct sockaddr_in addr = {AF_INET, htons(t_port)};
            inet_pton(AF_INET, t_ip, &addr.sin_addr);
            if(connect(s, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
                memset(buffer, 0, 1024);
                recv(s, buffer, 1024, 0);
                printf("\033[1;32m[DOWNLOAD_COMPLETE] Content: %s\033[0m\n", buffer);
                close(s);
                // Report to Server so it shows up in your logs
                int rs = socket(AF_INET, SOCK_STREAM, 0);
                struct sockaddr_in s_addr = {AF_INET, htons(INDEX_PORT)};
                inet_pton(AF_INET, SERVER_IP, &s_addr.sin_addr);
                if(connect(rs, (struct sockaddr *)&s_addr, sizeof(s_addr)) == 0) {
                    sprintf(buffer, "REPORT %s", file);
                    send(rs, buffer, strlen(buffer), 0);
                }
                close(rs);
            } else printf("\033[0;31m[FAILED] Peer at %s:%d is unreachable.\033[0m\n", t_ip, t_port);
        }
    }
    return 0;
}
