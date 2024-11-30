#include "http_handler.h"
#include "sse.h"
#include <json-c/json.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>

void *handle_request(void *args) {
    pthread_detach(pthread_self());
    ThreadArgs* tArgs = (ThreadArgs*)args;
    int client_sock = tArgs->connfd;
    struct sockaddr_in client_addr = tArgs->cliaddr;
    free(tArgs);

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

    buffer[received_bytes] = '\0';

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
        SseArgs* args = (SseArgs*)malloc(sizeof(SseArgs));
        args->connfd = client_sock;
        args->cliaddr = client_addr;

        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_sse_client, (void *)args) != 0) {
            perror("Error creating thread");
        }
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