// #include "handler/http_handler.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "core/server.h"

#define PORT 8080

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);

    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Error creating socket");
        exit(0);
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(server_addr.sin_zero), 8);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error binding socket");
        exit(0);
    }

    if (listen(server_sock, 10) == -1) {
        perror("Error listening on socket");
        exit(0);
    }
    printf("Server listening on port %d\n", PORT);

    while (1) {
        if ((client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_size)) == -1) {
            perror("Error accepting client");
            continue;
        }

        // Step 5: Create a new thread for each client
        ThreadArgs* args = (ThreadArgs*)malloc(sizeof(ThreadArgs));
        args->connfd = client_sock;
        args->cliaddr = client_addr;

        pthread_t tid;

        if (pthread_create(&tid, NULL, handle_request, (void *)args) != 0) {
            perror("Error creating thread.");
        } 
    }

    close(server_sock);
    return 0;
}
