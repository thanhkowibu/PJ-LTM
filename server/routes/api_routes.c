#include "api_routes.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <json-c/json.h>
#include "sse.h"

#define MAX_CLIENTS 100
extern Client clients[MAX_CLIENTS];
extern fd_set master_set;

void set_option(int client_sock, const char *request, const char *body) {
    const char *response =
        "HTTP/1.1 204 No Content\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "Connection: keep-alive\r\n\r\n";
    send(client_sock, response, strlen(response), 0);
}

void subcribe(int client_sock, const char *request, const char *body) {
    // printf("Subscribing client: %d\n", client_sock);
    const char *sse_headers =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/event-stream\r\n"
        "Cache-Control: no-cache\r\n"
        "Connection: keep-alive\r\n"
        "Access-Control-Allow-Origin: *\r\n\r\n";
    send(client_sock, sse_headers, strlen(sse_headers), 0);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].client_sock == client_sock) {
            clients[i].is_sse = true;
            printf("Client %d subscribed to SSE\n", client_sock);
            break;
        }
    }

}

void get_data(int client_sock, const char *request, const char *body) {
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
    json_object_put(json_response);
}

void send_message(int client_sock, const char *request, const char *body) {
    struct json_object *json_request = json_tokener_parse(body);
    struct json_object *message_obj;

    const char *message_str = "No message received";
    if (json_request && json_object_object_get_ex(json_request, "message", &message_obj)) {
        message_str = json_object_get_string(message_obj);
    }

    broadcast_message(message_str, client_sock);

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

    json_object_put(json_request);
    json_object_put(json_response);
}

void choice(int client_sock, const char *request, const char *body) {
    // Similar logic for choice endpoint
    printf("Received JSON payload: %s\n", body);

    struct json_object *json_request = json_tokener_parse(body);
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