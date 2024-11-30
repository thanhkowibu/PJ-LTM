#ifndef HTTP_HANDLER_H
#define HTTP_HANDLER_H
#include <netinet/in.h>

typedef struct ThreadArgs {
    int connfd;
    struct sockaddr_in cliaddr;
} ThreadArgs;

void *handle_request(void *arg);

#endif // HTTP_HANDLER_H