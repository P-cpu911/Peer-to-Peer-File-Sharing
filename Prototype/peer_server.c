#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 9000
#define BUFFER_SIZE 1024

int main(){
    int server_fd, new_socket;
    struct sockaddr_in address;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 3);

    printf("Peer server running on port %d\n", PORT);

    while(1){
        new_socket = accept(server_fd, NULL, NULL);

        if(new_socket < 0) continue;

        char filename[256];
        int len = read(new_socket, filename, sizeof(filename)-1);

        if(len <= 0){
            close(new_socket);
            continue;
        }

        filename[len] = '\0';
        filename[strcspn(filename, "\n")] = 0;

        printf("Request file: %s\n", filename);

        FILE* file = fopen(filename, "rb");

        if(!file){
            printf("File not found\n");
            close(new_socket);
            continue;
        }

        char buffer[BUFFER_SIZE];
        size_t bytes;

        while((bytes = fread(buffer, 1, BUFFER_SIZE, file)) > 0){
            send(new_socket, buffer, bytes, 0);
        }

        fclose(file);
        close(new_socket);

        printf("File sent\n");
    }
}
