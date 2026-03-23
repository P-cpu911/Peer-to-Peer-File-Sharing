#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8784
#define MAX 100

typedef struct {
    char filename[256];
    char ip[64];
    int port;
} FileEntry;

FileEntry db[MAX];
int dbSize = 0;

int main(){
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[1024];

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 5);

    printf("Index server started on port %d\n", PORT);

    while(1){
        new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);

        memset(buffer, 0, sizeof(buffer));
        read(new_socket, buffer, sizeof(buffer));

        printf("Received: %s\n", buffer);

        char command[16], filename[256], ip[64];
        int port;

        sscanf(buffer, "%s %s %s %d", command, filename, ip, &port);

        // SEED command
        if(strcmp(command, "SEED") == 0){
            strcpy(db[dbSize].filename, filename);
            strcpy(db[dbSize].ip, ip);
            db[dbSize].port = port;
            dbSize++;

            char reply[] = "OK\n";
            send(new_socket, reply, strlen(reply), 0);
        }

        // SEARCH command
        else if(strcmp(command, "SEARCH") == 0){
            int found = 0;

            for(int i = 0; i < dbSize; i++){
                if(strcmp(db[i].filename, filename) == 0){
                    char reply[256];
                    sprintf(reply, "%s %d\n", db[i].ip, db[i].port);
                    send(new_socket, reply, strlen(reply), 0);
                    found = 1;
                    break;
                }
            }

            if(!found){
                char reply[] = "NOTFOUND\n";
                send(new_socket, reply, strlen(reply), 0);
            }
        }

        close(new_socket);
    }

    return 0;
}
