#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

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
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

int findFile(char* filename){
    for(int i = 0; i < dbSize; i++){
        if(strcmp(db[i].filename, filename) == 0) return i;
    }
    return -1;
}

void* handle_client(void* arg) {
    int new_socket = *(int*)arg;
    free(arg);
    char buffer[1024] = {0};
    
    int valread = read(new_socket, buffer, sizeof(buffer)-1);
    if (valread <= 0) { close(new_socket); return NULL; }

    char command[16], filename[256], ip[64];
    int port, size;
    int items = sscanf(buffer, "%s %s %d %s %d", command, filename, &size, ip, &port);

    pthread_mutex_lock(&lock);
    if(strcmp(command, "SEED") == 0 && items >= 5){
        int idx = findFile(filename);
        if(idx == -1){
            idx = dbSize++;
            strcpy(db[idx].filename, filename);
            db[idx].size = size;
            db[idx].peerCount = 0;
        }
        int exists = 0;
        for(int i=0; i<db[idx].peerCount; i++) {
            if(db[idx].peers[i].port == port && strcmp(db[idx].peers[i].ip, ip) == 0) exists = 1;
        }
        if(!exists && db[idx].peerCount < MAX_PEERS){
            strcpy(db[idx].peers[db[idx].peerCount].ip, ip);
            db[idx].peers[db[idx].peerCount].port = port;
            db[idx].peerCount++;
        }
        printf("[INDEX] Registered: %s (%d bytes) from %s:%d\n", filename, size, ip, port);
        send(new_socket, "OK\n", 3, 0);
    } 
    else if(strcmp(command, "SEARCH") == 0){
        char reply[2048] = "";
        for(int i = 0; i < dbSize; i++){
            if(strstr(db[i].filename, filename) != NULL){
                for(int j = 0; j < db[i].peerCount; j++){
                    char line[256];
                    sprintf(line, "%s %d -> %s:%d\n", db[i].filename, db[i].size, db[i].peers[j].ip, db[i].peers[j].port);
                    strcat(reply, line);
                }
            }
        }
        if(strlen(reply) == 0) strcpy(reply, "NOTFOUND\n");
        send(new_socket, reply, strlen(reply), 0);
    }
    pthread_mutex_unlock(&lock);
    close(new_socket);
    return NULL;
}

int main(){
    int server_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 10);
    printf("Index server started on port %d\n", PORT);

    while(1){
        int* new_sock = malloc(sizeof(int));
        *new_sock = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, new_sock);
        pthread_detach(tid);
    }
    return 0;
}
