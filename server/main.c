#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>

#include "core/server.h"
#include "core/sse.h"

#include "features/user.h"

#define PORT 8080

Client clients[MAX_CLIENTS];
fd_set master_set, read_fds;
int max_fd;

void init_clients() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].client_sock = -1;
        clients[i].is_sse = false;
    }
}

int main() {
    int server_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addrlen = sizeof(client_addr);

    init_clients();

    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    if (listen(server_sock, 10) < 0) {
        perror("Listen failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }
    
    _initUser();

    printf("Server listening on port %d\n", PORT);

    FD_ZERO(&master_set);
    FD_SET(server_sock, &master_set);
    max_fd = server_sock;

    while (1) {
        read_fds = master_set;

        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (activity < 0) {
            perror("Select error");
            break;
        }

        if (FD_ISSET(server_sock, &read_fds)) {
            int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addrlen);
            if (client_sock < 0) {
                perror("Accept error");
                continue;
            }

            printf("New client connected: %d\n", client_sock);

            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].client_sock == -1) {
                    clients[i].client_sock = client_sock;
                    FD_SET(client_sock, &master_set);
                    if (client_sock > max_fd) {
                        max_fd = client_sock;
                    }
                    break;
                }
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].client_sock != -1 && FD_ISSET(clients[i].client_sock, &read_fds)) {
                handle_request(clients[i].client_sock);
            }
        }
    }


    _cleanUser();
    close(server_sock);
    return 0;
}
