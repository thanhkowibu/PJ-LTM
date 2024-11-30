#ifndef SSE_H
#define SSE_H

#define MAX_CLIENTS 20
#define BUFF_SIZE 4096
#include <netinet/in.h>

typedef struct SseArgs {
    int connfd;
    struct sockaddr_in cliaddr;
} SseArgs;

void add_sse_client(int client_sock);
void remove_sse_client(int client_sock);
void broadcast_message(const char *message);
void *handle_sse_client(void *args);

#endif // SSE_H