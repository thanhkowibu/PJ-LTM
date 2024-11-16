#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <json-c/json.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT 8080
#define BUFF_SIZE 4096
#define MAX_CLIENTS 20

int sse_clients[MAX_CLIENTS]; // Store connected SSE client sockets
int sse_client_count = 0;

void add_sse_client(int client_sock) {
    if (sse_client_count < MAX_CLIENTS) {
        sse_clients[sse_client_count++] = client_sock;
    } else {
        close(client_sock); // Close if we can't handle more clients
    }
}

void remove_sse_client(int client_sock) {
    for (int i = 0; i < sse_client_count; i++) {
        if (sse_clients[i] == client_sock) {
            // Shift remaining clients down the list
            for (int j = i; j < sse_client_count - 1; j++) {
                sse_clients[j] = sse_clients[j + 1];
            }
            sse_client_count--;
            break;
        }
    }
}

void broadcast_message(const char *message) {
    char response[BUFF_SIZE];
    snprintf(response, sizeof(response), "data: %s\n\n", message);

    for (int i = 0; i < sse_client_count; i++) {
        send(sse_clients[i], response, strlen(response), 0);
    }
}

void handle_request(int client_sock, struct sockaddr_in client_addr) {
    char buffer[BUFF_SIZE];
    int received_bytes = recv(client_sock, buffer, BUFF_SIZE - 1, 0);

    if (received_bytes < 0) {
        perror("Error receiving data");
        close(client_sock);
        return;
    }

    buffer[received_bytes] = '\0'; // Null terminate the received data

    // Check if this is a preflight (OPTIONS) request for CORS
    if (strncmp(buffer, "OPTIONS", 7) == 0) {
        char response[] =
            "HTTP/1.1 204 No Content\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
            "Access-Control-Allow-Headers: Content-Type\r\n"
            "Connection: keep-alive\r\n\r\n";
        send(client_sock, response, sizeof(response) - 1, 0);
    } else if (strncmp(buffer, "GET /api/data", 13) == 0) {
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
    } else if (strncmp(buffer, "GET /api/subscribe", 18) == 0) { 
        char response[] =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/event-stream\r\n"
            "Cache-Control: no-cache\r\n"
            "Connection: keep-alive\r\n"
            "Access-Control-Allow-Origin: *\r\n\r\n"; // Make sure CORS header is here
        send(client_sock, response, sizeof(response) - 1, 0);
        add_sse_client(client_sock);
        return; // Keep the connection open for SSE
    }
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
        broadcast_message(message_str);

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
    } else if (strncmp(buffer, "POST /api/choice", 16) == 0) {
        // Similar logic for choice endpoint
        char *json_start = strstr(buffer, "\r\n\r\n");
        if (json_start) {
            json_start += 4;
        } else {
            json_start = buffer;
        }

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

        send(client_sock, response, strlen(response), 0);

        json_object_put(json_request);
        json_object_put(json_response);
    } else {
        char error_response[] = "HTTP/1.1 404 Not Found\r\n\r\n";
        send(client_sock, error_response, sizeof(error_response) - 1, 0);
    }

    close(client_sock);
}

void sig_chld(int signo) {
    pid_t pid;
    int stat;
    pid = waitpid(-1, &stat, WNOHANG);
    printf("Child %d terminated\n", pid);
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);
    pid_t pid;

    // Step 1: Construct a TCP socket
    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("\nError: ");
        exit(0);
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(server_addr.sin_zero), 8);

    // Step 2: Bind address to socket
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("\nError: ");
        exit(0);
    }

    // Step 3: Listen for incoming connections
    if (listen(server_sock, 5) == -1) {
        perror("\nError: ");
        exit(0);
    }
    printf("Server listening on port %d\n", PORT);

    // Wait for a child process to stop
    signal(SIGCHLD, sig_chld);

    // Step 4: Accept connection from a client
    while (1) {
        if ((client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_size)) == -1) {
            perror("\nError accepting client: ");
            continue;
        }

        // Step 5: Handle each client in child process
        if ((pid = fork()) == 0) {
            close(server_sock);
            handle_request(client_sock, client_addr);

            // Close client socket
            close(client_sock);
            printf("Client %s:%d disconnected\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            exit(0);
        } else if (pid < 0) {
            perror("Fork failed");
            exit(EXIT_FAILURE);
        }

        close(client_sock);
    }

    close(server_sock);
    return 0;
}
