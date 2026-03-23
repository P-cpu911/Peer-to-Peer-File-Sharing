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
#define MAX_CLIENT 10
#define BUFFER_SIZE 1024

typedef struct {
    char filename[256];
    char ip[64];
    int port;
} FileEntry;

// SIMPLE STATIC DB
FileEntry db[] = {
    {"test.txt", "127.0.0.1", 9000},
    {"testpic.png", "127.0.0.1", 9000}
};

int dbSize = 2;

int main(){
    int server_fd, new_socket, clientFDs[MAX_CLIENT];
    struct sockaddr_in address;

    fd_set readfds;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 3);

    memset(clientFDs, 0, sizeof(clientFDs));

    printf("Index server started on port %d\n", PORT);

    while(1){
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        int max_fd = server_fd;

        for(int i=0;i<MAX_CLIENT;i++){
            if(clientFDs[i] > 0){
                FD_SET(clientFDs[i], &readfds);
                if(clientFDs[i] > max_fd)
                    max_fd = clientFDs[i];
            }
        }

        select(max_fd + 1, &readfds, NULL, NULL, NULL);

        if(FD_ISSET(server_fd, &readfds)){
            new_socket = accept(server_fd, NULL, NULL);

            for(int i=0;i<MAX_CLIENT;i++){
                if(clientFDs[i] == 0){
                    clientFDs[i] = new_socket;
                    break;
                }
            }
        }

        for(int i=0;i<MAX_CLIENT;i++){
            int sd = clientFDs[i];
            if(sd > 0 && FD_ISSET(sd, &readfds)){
                char buffer[BUFFER_SIZE];
                int valread = read(sd, buffer, BUFFER_SIZE-1);

                if(valread <= 0){
                    close(sd);
                    clientFDs[i] = 0;
                }
                else{
                    buffer[valread] = '\0';

                    // remove newline
                    buffer[strcspn(buffer, "\n")] = 0;

                    printf("SEARCH: %s\n", buffer);

                    int found = 0;

                    for(int j=0;j<dbSize;j++){
                        if(strcmp(buffer, db[j].filename) == 0){
                            char reply[128];
                            sprintf(reply, "%s %d\n", db[j].ip, db[j].port);
                            send(sd, reply, strlen(reply), 0);
                            found = 1;
                            break;
                        }
                    }

                    if(!found){
                        send(sd, "NOTFOUND\n", 9, 0);
                    }
                }
            }
        }
    }
}
