#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8784
#define MAX_FILES 100
#define MAX_PEERS 10

typedef struct {
    char ip[64];
    int port;
} Peer;

typedef struct {
    char filename[256];
    int size;
    Peer peers[MAX_PEERS];
    int peerCount;
} FileEntry;

FileEntry db[MAX_FILES];
int dbSize = 0;

// find file
int findFile(char* filename){
    for(int i = 0; i < dbSize; i++){
        if(strcmp(db[i].filename, filename) == 0)
            return i;
    }
    return -1;
}

// check duplicate peer
int peerExists(int idx, char* ip, int port){
    for(int i = 0; i < db[idx].peerCount; i++){
        if(strcmp(db[idx].peers[i].ip, ip) == 0 &&
           db[idx].peers[i].port == port)
            return 1;
    }
    return 0;
}

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

        char command[16], filename[256], ip[64];
        int port, size;

        sscanf(buffer, "%s %s %d %s %d", command, filename, &size, ip, &port);

        // SEED
        if(strcmp(command, "SEED") == 0){
            int idx = findFile(filename);

            if(idx == -1){
                idx = dbSize++;
                strcpy(db[idx].filename, filename);
                db[idx].size = size;
                db[idx].peerCount = 0;
            }

            if(!peerExists(idx, ip, port)){
                int p = db[idx].peerCount;
                strcpy(db[idx].peers[p].ip, ip);
                db[idx].peers[p].port = port;
                db[idx].peerCount++;
            }

            printf("Registered: %s (%d bytes)\n", filename, size);

            send(new_socket, "OK\n", 3, 0);
        }

        // SEARCH
        else if(strcmp(command, "SEARCH") == 0){
            char reply[2048] = "";

            for(int i = 0; i < dbSize; i++){
                if(strstr(db[i].filename, filename) != NULL){
                    for(int j = 0; j < db[i].peerCount; j++){
                        char line[256];
                        sprintf(line, "%s %d -> %s:%d\n",
                            db[i].filename,
                            db[i].size,
                            db[i].peers[j].ip,
                            db[i].peers[j].port);
                        strcat(reply, line);
                    }
                }
            }

            if(strlen(reply) == 0){
                strcpy(reply, "NOTFOUND\n");
            }

            send(new_socket, reply, strlen(reply), 0);
        }

        close(new_socket);
    }

    return 0;
}
