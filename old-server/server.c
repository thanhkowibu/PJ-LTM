#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>

#define PORT 8080
#define MAX_CLIENTS 100
#define BUFF_SIZE 1024

typedef struct {
    int sockfd;
    bool is_sse;  // Flag to check if this client is subscribed to SSE
} Client;

Client clients[MAX_CLIENTS];
fd_set master_set, read_fds;
int max_fd;

void init_clients() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].sockfd = -1;
        clients[i].is_sse = false;
    }
}

void remove_client(int sockfd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].sockfd == sockfd) {
            close(clients[i].sockfd);
            clients[i].sockfd = -1;
            clients[i].is_sse = false;
            FD_CLR(sockfd, &master_set);
            printf("Client disconnected: %d\n", sockfd);
            break;
        }
    }
}

void broadcast_message(const char *message, int sender_sock) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].sockfd != -1 && clients[i].is_sse && clients[i].sockfd != sender_sock) {
            char sse_message[BUFF_SIZE];
            snprintf(sse_message, sizeof(sse_message), "data: %s\n\n", message);
            send(clients[i].sockfd, sse_message, strlen(sse_message), 0);
        }
    }
}

void handle_http_request(int sockfd) {
    char buffer[BUFF_SIZE];
    int bytes_received = recv(sockfd, buffer, BUFF_SIZE - 1, 0);

    if (bytes_received <= 0) {
        remove_client(sockfd);
        return;
    }

    buffer[bytes_received] = '\0';

    // Check if the request is for SSE subscription
    if (strstr(buffer, "GET /api/subscribe")) {
        const char *sse_headers =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/event-stream\r\n"
            "Cache-Control: no-cache\r\n"
            "Connection: keep-alive\r\n"
            "Access-Control-Allow-Origin: *\r\n\r\n";
        send(sockfd, sse_headers, strlen(sse_headers), 0);

        // Mark the client as subscribed to SSE
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].sockfd == sockfd) {
                clients[i].is_sse = true;
                printf("Client %d subscribed to SSE\n", sockfd);
                break;
            }
        }
    }
    // Check if the request is for sending a message
    else if (strstr(buffer, "POST /api/message")) {
        // Extract the message body
        char *message_start = strstr(buffer, "\r\n\r\n");
        if (message_start) {
            message_start += 4;  // Skip the "\r\n\r\n"
            printf("Received message: %s\n", message_start);
            broadcast_message(message_start, sockfd);
        }

        const char *response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Access-Control-Allow-Methods: POST, GET, OPTIONS\r\n"
            "Access-Control-Allow-Headers: Content-Type\r\n\r\n"
            "{\"status\": \"Message broadcasted\"}";
        send(sockfd, response, strlen(response), 0);
    }
    // Handle OPTIONS request for CORS preflight
    else if (strstr(buffer, "OPTIONS /api/message")) {
        const char *response =
            "HTTP/1.1 204 No Content\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Access-Control-Allow-Methods: POST, GET, OPTIONS\r\n"
            "Access-Control-Allow-Headers: Content-Type\r\n\r\n";
        send(sockfd, response, strlen(response), 0);
    }
    // Default response for unsupported endpoints
    else {
        const char *response =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/plain\r\n"
            "Access-Control-Allow-Origin: *\r\n\r\n"
            "Endpoint not found.";
        send(sockfd, response, strlen(response), 0);
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
                if (clients[i].sockfd == -1) {
                    clients[i].sockfd = client_sock;
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
            if (clients[i].sockfd != -1 && FD_ISSET(clients[i].sockfd, &read_fds)) {
                handle_http_request(clients[i].sockfd);
            }
        }
    }

    // Cleanup
    close(server_sock);
    return 0;
}
