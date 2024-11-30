#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <json-c/json.h>
#include <pthread.h>

#define PORT 8080
#define BUFF_SIZE 4096
#define MAX_CLIENTS 20

int sse_clients[MAX_CLIENTS]; // Store connected SSE client sockets
int sse_client_count = 0;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void add_sse_client(int client_sock) {
    pthread_mutex_lock(&clients_mutex);
    if (sse_client_count < MAX_CLIENTS) {
        sse_clients[sse_client_count++] = client_sock;
    } else {
        close(client_sock); // Close if we can't handle more clients
    }
    pthread_mutex_unlock(&clients_mutex);
}

void remove_sse_client(int client_sock) {
    pthread_mutex_lock(&clients_mutex);
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
            i--; // Adjust index after removal
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void *handle_sse_client(void *arg) {
    int client_sock = *((int *)arg);
    free(arg);

    // Keep the connection open for SSE
    while (1) {
        // SSE clients expect periodic keep-alive messages
        char keep_alive[] = ":\n\n";
        if (send(client_sock, keep_alive, sizeof(keep_alive) - 1, 0) < 0) {
            perror("Error sending keep-alive message");
            break;
        }
        sleep(15); // Send keep-alive every 15 seconds
    }

    // Remove client from the list when done
    remove_sse_client(client_sock);
    close(client_sock);
    pthread_exit(NULL);
}

void *handle_request(void *arg) {
    int client_sock = *((int *)arg);
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    getpeername(client_sock, (struct sockaddr *)&client_addr, &addr_len);
    free(arg);

    char buffer[BUFF_SIZE];
    int received_bytes = recv(client_sock, buffer, BUFF_SIZE - 1, 0);

    if (received_bytes < 0) {
        perror("Error receiving data");
        close(client_sock);
        return NULL;
    } else if (received_bytes == 0) {
        printf("Client disconnected.\n");
        close(client_sock);
        return NULL;
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

        // Create a new thread to handle the SSE client
        pthread_t tid;
        int *new_sock = malloc(sizeof(int));
        *new_sock = client_sock;
        if (pthread_create(&tid, NULL, handle_sse_client, (void *)new_sock) != 0) {
            perror("Error creating thread");
            free(new_sock);
        }
        pthread_detach(tid); // Detach the thread to avoid resource leaks
        return NULL; // Return to avoid closing the client socket
    } else if (strncmp(buffer, "POST /api/message", 17) == 0) {
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
    return NULL;
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);

    // Step 1: Construct a TCP socket
    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Error creating socket");
        exit(0);
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(server_addr.sin_zero), 8);

    // Step 2: Bind address to socket
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error binding socket");
        exit(0);
    }

    // Step 3: Listen for incoming connections
    if (listen(server_sock, 10) == -1) {
        perror("Error listening on socket");
        exit(0);
    }
    printf("Server listening on port %d\n", PORT);

    while (1) {
        // Step 4: Accept connection from a client
        if ((client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_size)) == -1) {
            perror("Error accepting client");
            continue;
        }

        // Step 5: Handle each client in a new thread
        pthread_t tid;
        int *new_sock = malloc(sizeof(int));
        if (new_sock == NULL) {
            perror("Error allocating memory for new socket");
            close(client_sock);
            continue;
        }

        *new_sock = client_sock;
        if (pthread_create(&tid, NULL, handle_request, (void *)new_sock) != 0) {
            perror("Error creating thread");
            free(new_sock);
            close(client_sock);
        } else {
            pthread_detach(tid); // Detach thread to avoid memory leaks
        }
    }


    close(server_sock);
    return 0;
}