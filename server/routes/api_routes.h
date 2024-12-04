#ifndef API_ROUTER_H
#define API_ROUTER_H

#include "sse.h"

void subcribe(int client_sock, const char *request, const char *body);
void get_data(int client_sock, const char *request, const char *body);
void send_message(int client_sock, const char *request, const char *body);
void choice(int client_sock, const char *request, const char *body);
void set_option(int client_sock, const char *request, const char *body);

#endif