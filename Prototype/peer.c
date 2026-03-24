#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/stat.h>

#define INDEX_PORT 8784
#define PEER_PORT 9000
#define BUFFER 4096

int getFileSize(char* filename){
    struct stat st;
    if(stat(filename, &st) == 0) return st.st_size;
    return -1;
}

void* server_thread(void* arg){
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PEER_PORT);

    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 5);
    printf("Listening for downloads on port %d...\n", PEER_PORT);

    while(1){
        new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        char filename[256] = {0};
        int n = recv(new_socket, filename, sizeof(filename)-1, 0);
        if(n > 0) {
            filename[strcspn(filename, "\r\n")] = 0;
            FILE* f = fopen(filename, "rb");
            if(!f){
                send(new_socket, "ERROR", 5, 0);
            } else {
                char buf[BUFFER];
                int bytes;
                while((bytes = fread(buf, 1, BUFFER, f)) > 0){
                    send(new_socket, buf, bytes, 0);
                }
                fclose(f);
            }
        }
        close(new_socket);
    }
}

int connect_to(char* ip, int port){
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv;
    struct timeval timeout;
    timeout.tv_sec = 3; // 3 second timeout for "Offline" check
    timeout.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    serv.sin_family = AF_INET;
    serv.sin_port = htons(port);
    if(inet_pton(AF_INET, ip, &serv.sin_addr) <= 0) return -1;

    if(connect(sock, (struct sockaddr*)&serv, sizeof(serv)) < 0) return -1;
    return sock;
}

int main(){
    pthread_t tid;
    pthread_create(&tid, NULL, server_thread, NULL);

    char input[256], cmd[64], arg[128];
    printf("Commands: SEED <file>, SEARCH <file>, GET <file>, EXIT\n");

    while(1){
        printf(">>> ");
        if(!fgets(input, sizeof(input), stdin)) break;
        int num = sscanf(input, "%s %s", cmd, arg);
        if(num < 1) continue;

        if(strcmp(cmd, "SEED") == 0){
            int size = getFileSize(arg);
            if(size < 0) { printf("File not found locally.\n"); continue; }
            int sock = connect_to("127.0.0.1", INDEX_PORT);
            char msg[512];
            // Replace 127.0.0.1 with your actual LAN IP for multi-computer demo
            sprintf(msg, "SEED %s %d 127.0.0.1 %d", arg, size, PEER_PORT);
            send(sock, msg, strlen(msg), 0);
            close(sock);
            printf("Registered %s (%d bytes)\n", arg, size);
        }
        else if(strcmp(cmd, "SEARCH") == 0){
            int sock = connect_to("127.0.0.1", INDEX_PORT);
            char msg[256];
            sprintf(msg, "SEARCH %s", arg);
            send(sock, msg, strlen(msg), 0);
            char res[2048] = {0};
            recv(sock, res, sizeof(res)-1, 0);
            printf("Results:\n%s", res);
            close(sock);
        }
        else if(strcmp(cmd, "GET") == 0){
            char ip[64]; int port;
            printf("Enter Peer IP and Port: ");
            scanf("%s %d", ip, &port); getchar();
            
            int sock = connect_to(ip, port);
            if(sock < 0) { printf("Error: Peer unreachable.\n"); continue; }
            
            send(sock, arg, strlen(arg), 0);
            FILE* f = fopen(arg, "wb");
            char buf[BUFFER]; int n, total = 0;
            while((n = recv(sock, buf, BUFFER, 0)) > 0){
                if(strncmp(buf, "ERROR", 5) == 0) { printf("Peer reported file error.\n"); break; }
                fwrite(buf, 1, n, f);
                total += n;
            }
            fclose(f); close(sock);
            printf("Download complete. Received %d bytes.\n", total);
        }
        else if(strcmp(cmd, "EXIT") == 0) exit(0);
    }
    return 0;
}
