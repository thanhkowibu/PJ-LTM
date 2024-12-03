#include "http_handler.h"
#include "sse.h"
#include "../features/user.h"
#include "../features/room.h"

#include <json-c/json.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>

/*
void *handle_request1(void *args) {
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
    } else if (strncmp(buffer, "POST /auth/login", 16) == 0) {
        // Locate the JSON body in the HTTP request
        char *json_start = strstr(buffer, "\r\n\r\n");
        if (json_start) {
            json_start += 4;
        } else {
            json_start = buffer;
        }
        printf("json_start: %s\n", json_start);
        struct json_object *json_request = json_tokener_parse(json_start);
        struct json_object *username_obj, *password_obj;

        const char *username = NULL;
        const char *password = NULL;
        printf("%d,%d\n", json_object_object_get_ex(json_request, "password", &username_obj), json_object_object_get_ex(json_request, "username", &password_obj));
        if (json_request && json_object_object_get_ex(json_request, "username", &username_obj) &&
            json_object_object_get_ex(json_request, "password", &password_obj)) {
            username = json_object_get_string(username_obj);
            password = json_object_get_string(password_obj);


            printf("u-%s,p-%s\n", username, password);
        }

        struct json_object *json_response = json_object_new_object();
        if (username && password && authenticate_user(username, password)) {
            json_object_object_add(json_response, "status", json_object_new_string("success"));
            json_object_object_add(json_response, "message", json_object_new_string("Login successful"));
        } else {
            json_object_object_add(json_response, "status", json_object_new_string("failure"));
            json_object_object_add(json_response, "message", json_object_new_string("Invalid credentials"));
        }

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

    } else if (strncmp(buffer, "POST /auth/signup", 17) == 0) {
        // Locate the JSON body in the HTTP requests
        char *json_start = strstr(buffer, "\r\n\r\n");
        if (json_start) {
            json_start += 4;
        } else {
            json_start = buffer;
        }

        printf("json_start: %s\n", json_start);
        struct json_object *json_request = json_tokener_parse(json_start);
        struct json_object *username_obj, *password_obj;

        const char *username = NULL;
        const char *password = NULL;
        if (json_request && json_object_object_get_ex(json_request, "username", &username_obj) &&
            json_object_object_get_ex(json_request, "password", &password_obj)) {
            username = json_object_get_string(username_obj);
            password = json_object_get_string(password_obj);

            printf("u-%s,p-%s\n", username, password);
        }

        struct json_object *json_response = json_object_new_object();
        // printf("%d\n", register_user(username, password));
        if (username && password && register_user(username, password)) {
            

            json_object_object_add(json_response, "status", json_object_new_string("success"));
            json_object_object_add(json_response, "message", json_object_new_string("Signup successful"));
        } else {
            json_object_object_add(json_response, "status", json_object_new_string("failure"));
            json_object_object_add(json_response, "message", json_object_new_string("Signup failed"));
        }

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

    } else if (strncmp(buffer, "GET /auth/logout", 16) == 0) {
        // Handle logout logic here
        struct json_object *json_response = json_object_new_object();

        
        if (log_out()) {
            json_object_object_add(json_response, "status", json_object_new_string("success"));
            json_object_object_add(json_response, "message", json_object_new_string("Logout successful"));
        } else {
            json_object_object_add(json_response, "status", json_object_new_string("failure"));
            json_object_object_add(json_response, "message", json_object_new_string("Already log out"));
        }

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
        json_object_put(json_response);

    } else if (strncmp(buffer, "POST /room/join", 15) == 0) {
        // Locate the JSON body in the HTTP request
        char *json_start = strstr(buffer, "\r\n\r\n");
        if (json_start) {
            json_start += 4;
        } else {
            json_start = buffer;
        }

        struct json_object *json_request = json_tokener_parse(json_start);
        struct json_object *room_name_obj;

        const char *room_name = NULL;
        if (json_request && json_object_object_get_ex(json_request, "room_name", &room_name_obj)) {
            room_name = json_object_get_string(room_name_obj);
        }

        struct json_object *json_response = json_object_new_object();
        printf("%d\n", (getCurUser() != NULL));
        if ((getCurUser() != NULL) && room_name && add_user_to_room(room_name, NULL)) {
            json_object_object_add(json_response, "status", json_object_new_string("success"));

            // Get the list of users in the room
            struct json_object *users_array = json_object_new_array();
            Room *room = get_room_by_name(room_name);
            if (room) {
                UserNode *current = room->users;
                while (current != NULL) {
                    struct json_object *user_obj = json_object_new_object();
                    json_object_object_add(user_obj, "username", json_object_new_string(current->user->username));
                    json_object_array_add(users_array, user_obj);
                    current = current->next;
                }
            }
            json_object_object_add(json_response, "users", users_array);
        } else {
            json_object_object_add(json_response, "status", json_object_new_string("failure"));
            json_object_object_add(json_response, "message", json_object_new_string("Room not found or room is full"));
        }
        
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

    } else if (strncmp(buffer, "POST /room/create", 17) == 0) {
        // Locate the JSON body in the HTTP request
        char *json_start = strstr(buffer, "\r\n\r\n");
        if (json_start) {
            json_start += 4;
        } else {
            json_start = buffer;
        }
        printf("json_start: %s\n", json_start);
        struct json_object *json_request = json_tokener_parse(json_start);
        struct json_object *room_name_obj, *capacity_obj;

        const char *room_name = NULL;
        int capacity = 0;
        if (json_request && json_object_object_get_ex(json_request, "room_name", &room_name_obj) &&
            json_object_object_get_ex(json_request, "capacity", &capacity_obj)) {
            room_name = json_object_get_string(room_name_obj);
            capacity = json_object_get_int(capacity_obj);
        }
        printf("%d,%d,%d\n", json_object_object_get_ex(json_request, "room_name", &room_name_obj), json_object_object_get_ex(json_request, "capacity", &capacity_obj),requiredLogin());
        struct json_object *json_response = json_object_new_object();
        if (room_name && capacity > 0 && create_room(room_name, capacity)) {
            json_object_object_add(json_response, "status", json_object_new_string("success"));
            json_object_object_add(json_response, "message", json_object_new_string("Room created successfully"));
        } else {
            json_object_object_add(json_response, "status", json_object_new_string("failure"));
            json_object_object_add(json_response, "message", json_object_new_string("Failed to create room"));
        }

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
*/