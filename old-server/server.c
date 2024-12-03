#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <json-c/json.h>

#define PORT 8081
#define MAX_CLIENTS 100
#define BUFF_SIZE 1024

typedef struct {
    int client_sock;
    bool is_sse;  // Flag to check if this client is subscribed to SSE
} Client;

Client clients[MAX_CLIENTS];
fd_set master_set, read_fds;
int max_fd;

void init_clients() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].client_sock = -1;
        clients[i].is_sse = false;
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

void broadcast_message(const char *message, int sender_sock) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].client_sock != -1 && clients[i].is_sse && clients[i].client_sock != sender_sock) {
            char sse_message[BUFF_SIZE];
            snprintf(sse_message, sizeof(sse_message), "data: %s\n\n", message);
            send(clients[i].client_sock, sse_message, strlen(sse_message), 0);
        }
    }
}

void handle_http_request(int client_sock) {
    char buffer[BUFF_SIZE];
    int bytes_received = recv(client_sock, buffer, BUFF_SIZE - 1, 0);

    if (bytes_received <= 0) {
        printf("Client disconnected: %d\n", client_sock);
        remove_client(client_sock);
        return;
    }

    buffer[bytes_received] = '\0';
    printf("Received request: %s\n", buffer);

    if (strncmp(buffer, "OPTIONS", 7) == 0) {
        char response[] =
            "HTTP/1.1 204 No Content\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
            "Access-Control-Allow-Headers: Content-Type\r\n"
            "Connection: keep-alive\r\n\r\n";
        send(client_sock, response, sizeof(response) - 1, 0);
    } else if (strstr(buffer, "GET /api/subscribe")) {
    // Check if the request is for SSE subscription
        const char *sse_headers =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/event-stream\r\n"
            "Cache-Control: no-cache\r\n"
            "Connection: keep-alive\r\n"
            "Access-Control-Allow-Origin: *\r\n\r\n";
        send(client_sock, sse_headers, strlen(sse_headers), 0);

        // Mark the client as subscribed to SSE
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].client_sock == client_sock) {
                clients[i].is_sse = true;
                printf("Client %d subscribed to SSE\n", client_sock);
                break;
            }
        }
    }
    else if (strncmp(buffer, "GET /api/data", 13) == 0) {
        // Handle GET /api/data
        struct json_object *json_response = json_object_new_object();
        json_object_object_add(json_response, "status", json_object_new_string("success"));
        json_object_object_add(json_response, "message", json_object_new_string("Hello, this is a JSON response"));

        const char *json_str = json_object_to_json_string(json_response);
        char response[BUFF_SIZE];
        snprintf(response, sizeof(response), 
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Content-Length: %zu\r\n"
            "Connection: keep-alive\r\n\r\n%s",
            strlen(json_str), json_str);

        send(client_sock, response, strlen(response), 0);
        json_object_put(json_response); // Free JSON object memory
    }
    else if (strncmp(buffer, "POST /api/choice", 16) == 0) {
        // Similar logic for choice endpoint
        char *json_start = strstr(buffer, "\r\n\r\n");
        if (json_start) {
            json_start += 4;
        } else {
            json_start = buffer;
        }

        printf("Received JSON payload: %s\n", json_start);

        struct json_object *json_request = json_tokener_parse(json_start);
        struct json_object *choice_obj;

        const char *choice_str = "No choice received";
        if (json_request && json_object_object_get_ex(json_request, "choice", &choice_obj)) {
            choice_str = json_object_get_string(choice_obj);
        }

        struct json_object *json_response = json_object_new_object();
        json_object_object_add(json_response, "status", json_object_new_string("Choice received"));
        json_object_object_add(json_response, "choice", json_object_new_string(choice_str));

        const char *json_str = json_object_to_json_string(json_response);
        char response[BUFF_SIZE];
        snprintf(response, sizeof(response),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Content-Length: %zu\r\n"
            "Connection: keep-alive\r\n\r\n%s",
            strlen(json_str), json_str);

        printf("Sending response: %s\n", response);
        send(client_sock, response, strlen(response), 0);

        json_object_put(json_request);
        json_object_put(json_response);
    }
    // Check if the request is for sending a message
    else if (strncmp(buffer, "POST /api/message", 17) == 0) {
        // Locate the JSON body in the HTTP request
        char *json_start = strstr(buffer, "\r\n\r\n");
        if (json_start) {
            json_start += 4; // Move past the \r\n\r\n to start of JSON data
        } else {
            json_start = buffer; // If no headers, assume the whole buffer is JSON
        }

        // Parse the JSON data
        struct json_object *json_request = json_tokener_parse(json_start);
        struct json_object *message_obj;

        const char *message_str = "No message received";
        if (json_request && json_object_object_get_ex(json_request, "message", &message_obj)) {
            message_str = json_object_get_string(message_obj);
        }

        // Broadcast the message to all SSE clients
        broadcast_message(message_str, client_sock);

        // Prepare the response
        struct json_object *json_response = json_object_new_object();
        json_object_object_add(json_response, "status", json_object_new_string("Message received"));
        json_object_object_add(json_response, "message", json_object_new_string(message_str));

        const char *json_str = json_object_to_json_string(json_response);
        char response[BUFF_SIZE];
        snprintf(response, sizeof(response),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Content-Length: %zu\r\n"
            "Connection: keep-alive\r\n\r\n%s",
            strlen(json_str), json_str);

        send(client_sock, response, strlen(response), 0);
        
        // Free JSON objects
        json_object_put(json_request);
        json_object_put(json_response);
    }
    // Handle OPTIONS request for CORS preflight
    else if (strstr(buffer, "OPTIONS /api/message")) {
        const char *response =
            "HTTP/1.1 204 No Content\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Access-Control-Allow-Methods: POST, GET, OPTIONS\r\n"
            "Access-Control-Allow-Headers: Content-Type\r\n\r\n";
        send(client_sock, response, strlen(response), 0);
    }
    // Default response for unsupported endpoints
    else {
        const char *response =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/plain\r\n"
            "Access-Control-Allow-Origin: *\r\n\r\n"
            "Endpoint not found.";
        send(client_sock, response, strlen(response), 0);
    }
}

int main() {
    int server_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addrlen = sizeof(client_addr);

    // Initialize client data
    init_clients();

    // Create socket
    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind socket
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(server_sock, 10) < 0) {
        perror("Listen failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    // Initialize select() sets
    FD_ZERO(&master_set);
    FD_SET(server_sock, &master_set);
    max_fd = server_sock;

    while (1) {
        read_fds = master_set;

        // Wait for activity using select()
        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (activity < 0) {
            perror("Select error");
            break;
        }

        // Handle new connections
        if (FD_ISSET(server_sock, &read_fds)) {
            int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addrlen);
            if (client_sock < 0) {
                perror("Accept error");
                continue;
            }

            printf("New client connected: %d\n", client_sock);

            // Add client to the list
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

        // Handle data from existing clients
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].client_sock != -1 && FD_ISSET(clients[i].client_sock, &read_fds)) {
                handle_http_request(clients[i].client_sock);
            }
        }
    }

    // Cleanup
    close(server_sock);
    return 0;
}
