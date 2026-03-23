#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>

#define INDEX_PORT 8784
#define PEER_PORT 9000
#define BUFFER 1024

// File size
int getFileSize(char* filename){
    FILE* f = fopen(filename, "rb");
    if(!f) return -1;
    fseek(f, 0, SEEK_END);
    int size = ftell(f);
    fclose(f);
    return size;
}

// Server thread
void* server_thread(void* arg){
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char filename[256];

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PEER_PORT);

    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 5);

    printf("Peer server running on %d\n", PEER_PORT);

    while(1){
        new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);

        read(new_socket, filename, sizeof(filename));
        filename[strcspn(filename, "\n")] = '\0';

        FILE* f = fopen(filename, "rb");

        if(!f){
            send(new_socket, "ERROR\n", 6, 0);
        } else {
            char buf[BUFFER];
            int n;
            while((n = fread(buf, 1, BUFFER, f)) > 0){
                send(new_socket, buf, n, 0);
            }
            fclose(f);
        }

        close(new_socket);
    }
}

// Connect helper
int connect_to(char* ip, int port){
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv;

    serv.sin_family = AF_INET;
    serv.sin_port = htons(port);
    inet_pton(AF_INET, ip, &serv.sin_addr);

    if(connect(sock, (struct sockaddr*)&serv, sizeof(serv)) < 0){
        return -1;
    }

    return sock;
}

int main(){
    pthread_t tid;
    pthread_create(&tid, NULL, server_thread, NULL);

    char input[256], cmd[64], arg[128];

    while(1){
        printf(">>> ");
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = 0;

        sscanf(input, "%s %s", cmd, arg);

        // Seed
        if(strcmp(cmd, "SEED") == 0){
            int size = getFileSize(arg);

            char msg[256];
            sprintf(msg, "SEED %s %d 127.0.0.1 %d\n", arg, size, PEER_PORT);

            int sock = connect_to("127.0.0.1", INDEX_PORT);
            send(sock, msg, strlen(msg), 0);

            char buf[128];
            recv(sock, buf, sizeof(buf), 0);
            printf("Index> %s", buf);

            close(sock);
        }

        // Search
        else if(strcmp(cmd, "SEARCH") == 0){
            char msg[256];
            sprintf(msg, "SEARCH %s\n", arg);

            int sock = connect_to("127.0.0.1", INDEX_PORT);
            send(sock, msg, strlen(msg), 0);

            char buf[2048];
            int n = recv(sock, buf, sizeof(buf)-1, 0);
            buf[n] = '\0';

            printf("%s", buf);

            close(sock);
        }

        // Download
        else if(strcmp(cmd, "GET") == 0){
            char ip[64];
            int port;

            printf("Enter peer IP and port: ");
            scanf("%s %d", ip, &port);
            getchar();

            int sock = connect_to(ip, port);
            if(sock < 0){
                printf("Error: Peer unreachable\n");
                continue;
            }

            send(sock, arg, strlen(arg), 0);

            FILE* f = fopen(arg, "wb");
            char buf[BUFFER];
            int n, total = 0;

            while((n = recv(sock, buf, BUFFER, 0)) > 0){
                fwrite(buf, 1, n, f);
                total += n;
            }

            fclose(f);
            close(sock);

            printf("Downloaded %d bytes\n", total);
        }

        else if(strcmp(cmd, "EXIT") == 0){
            break;
        }

        else{
            printf("Invalid command\n");
        }
    }

    return 0;
}
