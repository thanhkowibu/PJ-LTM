#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "sse.h"

void broadcast_message(const char *message, int sender_sock) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].client_sock != -1 && clients[i].is_sse && clients[i].client_sock != sender_sock) {
            char sse_message[BUFF_SIZE];
            snprintf(sse_message, sizeof(sse_message), "data: %s\n\n", message);
            send(clients[i].client_sock, sse_message, strlen(sse_message), 0);
        }
    }
}

void remove_client(int client_sock) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].client_sock == client_sock) {
            close(clients[i].client_sock);
            clients[i].client_sock = -1;
            clients[i].is_sse = false;
            FD_CLR(client_sock, &master_set);
            printf("Client disconnected: %d\n", client_sock);
            break;
        }
    }
}
