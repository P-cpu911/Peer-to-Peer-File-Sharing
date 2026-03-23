#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 9000
#define BUFFER_SIZE 1024

int main(){
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char filename[256];

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 5);

    printf("Peer server running on port %d\n", PORT);

    while(1){
        new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);

        memset(filename, 0, sizeof(filename));
        read(new_socket, filename, sizeof(filename));

        // remove newline
        filename[strcspn(filename, "\n")] = '\0';

        printf("Request file: %s\n", filename);

        FILE* file = fopen(filename, "rb");

        if(file == NULL){
            char msg[] = "ERROR\n";
            send(new_socket, msg, strlen(msg), 0);
        } else {
            char buffer[BUFFER_SIZE];
            int bytes;

            while((bytes = fread(buffer, 1, BUFFER_SIZE, file)) > 0){
                send(new_socket, buffer, bytes, 0);
            }

            fclose(file);
            printf("File sent\n");
        }

        close(new_socket);
    }

    return 0;
}
