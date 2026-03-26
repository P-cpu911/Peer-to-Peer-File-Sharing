#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8784
#define MAX_FILES 100

typedef struct {
    char filename[50];
    char ip[20];
    int port;
} FileEntry;

FileEntry registry[MAX_FILES];
int file_count = 0;

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024];

    printf("\033[0;36m[SYS_INIT] P2P Index Server starting on Port %d...\n", PORT);
    
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 5);

    printf("[SYS_READY] Waiting for peer connections...\033[0m\n\n");

    while(1) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        memset(buffer, 0, 1024);
        read(new_socket, buffer, 1024);

        if (strncmp(buffer, "SEED", 4) == 0) {
            sscanf(buffer, "SEED %s %d", registry[file_count].filename, &registry[file_count].port);
            strcpy(registry[file_count].ip, inet_ntoa(address.sin_addr));
            printf("\033[0;32m[LOG_DATA] SEED_SUCCESS: '%s' registered from %s:%d\033[0m\n", 
                   registry[file_count].filename, registry[file_count].ip, registry[file_count].port);
            file_count++;
        } 
        else if (strncmp(buffer, "SEARCH", 6) == 0) {
            char search_file[50];
            sscanf(buffer, "SEARCH %s", search_file);
            printf("[LOG_QUERY] Searching for: %s\n", search_file);
            
            int found = 0;
            for(int i=0; i<file_count; i++) {
                if(strcmp(registry[i].filename, search_file) == 0) {
                    sprintf(buffer, "FOUND %s %d", registry[i].ip, registry[i].port);
                    send(new_socket, buffer, strlen(buffer), 0);
                    found = 1;
                    break;
                }
            }
            if(!found) send(new_socket, "NOT_FOUND", 9, 0);
        }
        close(new_socket);
    }
    return 0;
}
