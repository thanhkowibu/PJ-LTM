#include "auth_routes.h"

#include "../features/user.h"
#include "../features/room.h"

#include "../utils/utils.h"

#include <json-c/json.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "../middleware/cookies.h"

#define BUFF_SIZE 4096


void handle_login(int client_sock, const char *request, const char * body) {
    struct json_object *json_request = json_tokener_parse(body);
    struct json_object *json_response = json_object_new_object();

    struct json_object *username_obj, *password_obj;

    const char *username = NULL;
    const char *password = NULL;

    printf("%d\n", check_cookies(request));

    if (check_cookies(request)) {
        json_object_object_add(json_response, "status", json_object_new_string("failure"));
        json_object_object_add(json_response, "message", json_object_new_string("Already logged in"));
        
        // setCurUser(find_user(extract_cookie(request, "session_id")));
        
        sendResponse(client_sock, json_object_to_json_string(json_response));
        return;
    }    
    printf("%d,%d\n", json_object_object_get_ex(json_request, "password", &username_obj), json_object_object_get_ex(json_request, "username", &password_obj));
    if (json_request && json_object_object_get_ex(json_request, "username", &username_obj) &&
        json_object_object_get_ex(json_request, "password", &password_obj)) {
        username = json_object_get_string(username_obj);
        password = json_object_get_string(password_obj);


        printf("u-%s,p-%s\n", username, password);
    }

    if (username && password && authenticate_user(username, password)) {
        json_object_object_add(json_response, "status", json_object_new_string("success"));
        json_object_object_add(json_response, "message", json_object_new_string("Login successful"));


        send_cookie_response(client_sock, json_object_to_json_string(json_response), username);
    } else {
        json_object_object_add(json_response, "status", json_object_new_string("failure"));
        json_object_object_add(json_response, "message", json_object_new_string("Invalid credentials"));

        sendResponse(client_sock, json_object_to_json_string(json_response));
    }

    
    json_object_put(json_request);
    json_object_put(json_response);
}

void handle_register(int client_sock, const char *request, const char * body) {
    struct json_object *json_request = json_tokener_parse(body);
    struct json_object *json_response = json_object_new_object();

    struct json_object *username_obj, *password_obj;

    const char *username = NULL;
    const char *password = NULL;
    if (json_request && json_object_object_get_ex(json_request, "username", &username_obj) &&
        json_object_object_get_ex(json_request, "password", &password_obj)) {
        username = json_object_get_string(username_obj);
        password = json_object_get_string(password_obj);

        printf("u-%s,p-%s\n", username, password);
    }

    
    // printf("%d\n", register_user(username, password));
    if (username && password && register_user(username, password)) {
        
        json_object_object_add(json_response, "status", json_object_new_string("success"));
        json_object_object_add(json_response, "message", json_object_new_string("Signup successful"));
    } else {
        json_object_object_add(json_response, "status", json_object_new_string("failure"));
        json_object_object_add(json_response, "message", json_object_new_string("Signup failed"));
    }

    sendResponse(client_sock, json_object_to_json_string(json_response));

    json_object_put(json_request);
    json_object_put(json_response);
}

void handle_logout(int client_sock, const char *request, const char *body) {
    struct json_object *json_response = json_object_new_object();

    // handle protected route

    if (check_cookies(request) && log_out()) {
        // delete_session(extract_cookie(request, "session_id"));
        json_object_object_add(json_response, "status", json_object_new_string("success"));
        json_object_object_add(json_response, "message", json_object_new_string("Logout successful"));
    } else {
        json_object_object_add(json_response, "status", json_object_new_string("failure"));
        json_object_object_add(json_response, "message", json_object_new_string("Already log out"));
    }

    sendResponse(client_sock, json_object_to_json_string(json_response));
    
    json_object_put(json_response);

}

void test(int client_sock, const char *request, const char *body) {
    

    check_cookies(request);
    char response[BUFF_SIZE];
    snprintf(response, sizeof(response),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Content-Length: %d\r\n"
        "Connection: keep-alive\r\n\r\n%s",
        4, "test");

    send(client_sock, response, strlen(response), 0);
}