#include "server.h"

#include "../features/user.h"
#include "../features/room.h"

// routes
#include "../routes/auth_routes.h"
#include "../routes/api_routes.h"
#include "../handler/http_handler.h"

#include <json-c/json.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>

#define BUFF_SIZE 4096

typedef struct {
    const char *method;               // HTTP method (e.g., GET, POST)
    const char *route;                // Request path (e.g., /api/login)
    void (*handler)(int, const char *, const char *); // Function pointer to handle the request
    int is_sse;                       // Flag for SSE routes (1 for SSE, 0 for RESTful)
} Route;

Route routes[] = {
    {"POST", "/auth/login", handle_login, 0},
    {"POST", "/auth/register", handle_register, 0},
    {"GET", "/auth/logout", handle_logout, 0},
    {"GET", "/test", test, 0},

    {"GET", "/api/subscribe", subcribe, 1},
    {"POST", "/api/choice", choice, 1},
    {"POST", "/api/message", send_message, 1},
    {"GET", "/api/data", get_data, 1},
    // {"OPTIONS", "/", set_option, 1},
    // {"GET", "/events", handle_sse_events, 1}, // SSE route
};


// middleware
void handle_request(int client_sock) {
    char buffer[BUFF_SIZE];
    char request[BUFF_SIZE];
    char json[BUFF_SIZE];

    int received_bytes = recv(client_sock, buffer, BUFF_SIZE - 1, 0);

    if (received_bytes <= 0) {
        perror("Error receiving data");
        remove_client(client_sock);
        return;
    } 

    buffer[received_bytes] = '\0';

    if (strncmp(buffer, "OPTIONS", 7) == 0) {
        const char *response =
            "HTTP/1.1 204 No Content\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
            "Access-Control-Allow-Headers: Content-Type\r\n"
            "Connection: keep-alive\r\n\r\n";
        send(client_sock, response, strlen(response), 0);
        return;
    }
    // Split buffer into request and json parts
    char *json_start = strstr(buffer, "\r\n\r\n");
    if (json_start != NULL) {
        json_start += 4; // Skip the "\r\n\r\n"
        strncpy(request, buffer, json_start - buffer);
        request[json_start - buffer] = '\0';
        strcpy(json, json_start);
    } else {
        strcpy(request, buffer);
        json[0] = '\0';
    }

    route_request(client_sock, request, json);
    // close(client_sock);
    return;
}

void parse_request(const char *request, char *method, char *path) {
    sscanf(request, "%s %s", method, path);
}


void route_request(int client_sock, const char *request, const char *json) {
    printf("%s-%s\n",request,json);
    char method[8], path[256];
    parse_request(request, method, path);

    // handle_http_request(client_sock);

    // Iterate through the routing table
    for (int i = 0; i < sizeof(routes) / sizeof(Route); i++) {
        // printf("method: %s, path: %s\n", method, path);
        if (strcmp(routes[i].method, method) == 0 && strcmp(routes[i].route, path) == 0) {
            if (routes[i].is_sse) {
                // Call SSE handler
                // printf("Routing to SSE handler for: %s %s\n", method, path);
                routes[i].handler(client_sock, request, json); // No body needed for SSE
            } else {
                // Call RESTful handler
                printf("Routing to RESTful handler for: %s %s\n", method, path);

                printf("json: %s\n", json);
                routes[i].handler(client_sock, request, json);
            }
            return;
        }
    }

    // If no match, send 404 response
    char *response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\nRoute not found";
    send(client_sock, response, strlen(response), 0);
}

int match_route(const char *route, const char *path, char *param) {
    // Example: "/api/room/:room_id" matches "/api/room/123"
    const char *colon = strchr(route, ':');
    if (colon) {
        size_t prefix_len = colon - route;
        if (strncmp(route, path, prefix_len) == 0 && path[prefix_len] == '/') {
            strcpy(param, path + prefix_len + 1); // Extract parameter (e.g., "123")
            return 1; // Match
        }
    }
    return strcmp(route, path) == 0; // Exact match
}

void send_error_response(int client_sock, int status_code, const char *message) {
    char response[512];
    snprintf(response, sizeof(response),
             "HTTP/1.1 %d %s\r\n"
             "Content-Type: text/plain\r\n"
             "\r\n"
             "%s",
             status_code, message, message);
    send(client_sock, response, strlen(response), 0);
    close(client_sock);
}
