#include "sse.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>

int sse_clients[MAX_CLIENTS];
int sse_client_count = 0;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void add_sse_client(int client_sock) {
    pthread_mutex_lock(&clients_mutex);
    if (sse_client_count < MAX_CLIENTS) {
        sse_clients[sse_client_count++] = client_sock;
    } else {
        close(client_sock);
    }
    pthread_mutex_unlock(&clients_mutex);
}

void remove_sse_client(int client_sock) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < sse_client_count; i++) {
        if (sse_clients[i] == client_sock) {
            for (int j = i; j < sse_client_count - 1; j++) {
                sse_clients[j] = sse_clients[j + 1];
            }
            sse_client_count--;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void broadcast_message(const char *message) {
    char response[BUFF_SIZE];
    snprintf(response, sizeof(response), "data: %s\n\n", message);

    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < sse_client_count; i++) {
        if (send(sse_clients[i], response, strlen(response), 0) < 0) {
            perror("Error sending message to client");
            remove_sse_client(sse_clients[i]);
            i--;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void *handle_sse_client(void *args) {
    pthread_detach(pthread_self());
    SseArgs* tArgs = (SseArgs*)args;
    int client_sock = tArgs->connfd;
    free(tArgs);

    while (1) {
        char keep_alive[] = ":\n\n";
        if (send(client_sock, keep_alive, sizeof(keep_alive) - 1, 0) < 0) {
            perror("Error sending keep-alive message");
            break;
        }
        sleep(15);
    }

    remove_sse_client(client_sock);
    close(client_sock);
    pthread_exit(NULL);
}