#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define BACKLOG 5
#define MAX_FILES 100

char INDEX_IP[50];
int INDEX_PORT;
int MY_P2P_PORT;
char seeded_files[MAX_FILES][256];
int seeded_count = 0;
char my_ip[20];

// --- SERVER SIDE ---
int is_seeded(const char *filename) {
    for (int i = 0; i < seeded_count; i++) {
        if (strcmp(seeded_files[i], filename) == 0) return 1;
    }
    return 0;
}

void send_file(int socket, const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        char msg[] = "[ERROR] File not found on seeder.";
        send(socket, msg, strlen(msg), 0);
        return;
    }
    char buffer[1024];
    size_t n;
    while ((n = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        send(socket, buffer, n, 0);
    }
    fclose(fp);
}

void *server_thread(void *arg) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(MY_P2P_PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind"); exit(1);
    }
    listen(server_fd, BACKLOG);

    while (1) {
        int new_socket = accept(server_fd, NULL, NULL);
        if (new_socket < 0) { perror("accept"); continue; }

        char request[256];
        int n = recv(new_socket, request, sizeof(request)-1, 0);
        if (n > 0) {
            request[n] = '\0';
            request[strcspn(request, "\r\n")] = '\0';
            if (is_seeded(request)) {
                send_file(new_socket, request);
            } else {
                char msg[] = "[ERROR] Requested file not seeded.";
                send(new_socket, msg, strlen(msg), 0);
            }
        }
        close(new_socket);
    }
    return NULL;
}

// --- CLIENT SIDE ---
void register_file(const char *file) {
    FILE *fp = fopen(file, "r");
    if (!fp) {
        printf("[ERROR] File '%s' does not exist.\n", file);
        return;
    }
    fclose(fp);

    int s = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {AF_INET, htons(INDEX_PORT)};
    inet_pton(AF_INET, INDEX_IP, &addr.sin_addr);

    if (connect(s, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
        char buffer[1024];
        sprintf(buffer, "SEED %s %d", file, MY_P2P_PORT);
        send(s, buffer, strlen(buffer), 0);
        printf("[SUCCESS] File registered on Index Server.\n");

        if (seeded_count < MAX_FILES) {
            strncpy(seeded_files[seeded_count], file, sizeof(seeded_files[0])-1);
            seeded_files[seeded_count][sizeof(seeded_files[0])-1] = '\0';
            seeded_count++;
        }
    } else {
        perror("[ERROR] Connect to Index Server failed");
    }
    close(s);
}

void get_my_ip() {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {AF_INET, htons(INDEX_PORT)};
    inet_pton(AF_INET, INDEX_IP, &addr.sin_addr);
    if (connect(s, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
        send(s, "HELLO", 5, 0);
        char buf[100]; int n = recv(s, buf, sizeof(buf)-1, 0);
        if (n > 0) {
            buf[n] = '\0';
            sscanf(buf, "YOUR_IP %s", my_ip);
        }
    }
    close(s);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s [Index_server_IP] [Index_server_port] [Peer_server_port]\n", argv[0]);
        return 1;
    }
    strncpy(INDEX_IP, argv[1], sizeof(INDEX_IP)-1);
    INDEX_PORT = atoi(argv[2]);
    MY_P2P_PORT = atoi(argv[3]);

    // Get our IP from Index Server
    get_my_ip();

    // Print startup banner
    printf("[SERVER] P2P NODE INITIALIZED\n");
    printf("IP: %s | PORT: %d | STATUS: SEEDING_ACTIVE\n\n", my_ip, MY_P2P_PORT);

    // Start server thread
    pthread_t tid;
    pthread_create(&tid, NULL, server_thread, NULL);

    // --- CLIENT INTERFACE ---
    char cmd[20], file[256], buffer[1024];
    while (1) {
        printf("Enter command (SEARCH <filename> / GET <filename> / SEED <filename> / exit): ");
        fflush(stdout);

        if (scanf("%19s", cmd) != 1) continue;

        if (strcasecmp(cmd, "SEARCH") == 0) {
            if (scanf("%255s", file) != 1) continue;
            int s = socket(AF_INET, SOCK_STREAM, 0);
            struct sockaddr_in addr = {AF_INET, htons(INDEX_PORT)};
            inet_pton(AF_INET, INDEX_IP, &addr.sin_addr);
            if (connect(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
                perror("[ERROR] Connect to Index Server failed");
                close(s); continue;
            }
            sprintf(buffer, "SEARCH %s", file);
            send(s, buffer, strlen(buffer), 0);
            memset(buffer, 0, 1024);
            int n = recv(s, buffer, 1024, 0);
            if (n > 0) {
                buffer[n] = '\0';
                printf("[INDEX_RESULT]\n%s\n", buffer);
            }
            close(s);
        }
        else if (strcasecmp(cmd, "GET") == 0) {
            if (scanf("%255s", file) != 1) continue;
            char target_ip[20]; int target_port;
            printf("Enter Peer IP and Port from SEARCH result: ");
            fflush(stdout);
            if (scanf("%19s %d", target_ip, &target_port) != 2) continue;

            int s = socket(AF_INET, SOCK_STREAM, 0);
            struct sockaddr_in addr = {AF_INET, htons(target_port)};
            inet_pton(AF_INET, target_ip, &addr.sin_addr);
            if (connect(s, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
                send(s, file, strlen(file), 0);
                FILE *fp = fopen(file, "wb");
                if (!fp) { printf("[ERROR] Could not create local file.\n"); close(s); continue; }
                int n, total = 0;
                while ((n = recv(s, buffer, sizeof(buffer)-1, 0)) > 0) {
                    buffer[n] = '\0';
                    if (total == 0 && strncmp(buffer, "[ERROR]", 7) == 0) {
                        printf("%s\n", buffer);
                        fclose(fp); remove(file); break;
                    }
                    fwrite(buffer, 1, n, fp);
                    total += n;
                }
                if (total > 0) printf("[DOWNLOAD_COMPLETE] Saved to %s\n", file);
                fclose(fp);
            } else perror("[ERROR] Connect to peer failed");
            close(s);
        }
        else if (strcasecmp(cmd, "SEED") == 0) {
            if (scanf("%255s", file) != 1) continue;
            register_file(file);
        }
        else if (strcasecmp(cmd, "exit") == 0) {
            printf("Shutting down peer...\n");
            exit(0);
        }
        else {
            printf("[ERROR] Unknown command. Use SEARCH, GET, SEED, or exit.\n");
        }
    }
    return 0;
}
