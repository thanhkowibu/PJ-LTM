#include "utils.h"

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
#include "../middleware/cookies.h"

#define BUFF_SIZE 4096

void sendResponse(int client_sock, const char *response) {
    char res[BUFF_SIZE];
    snprintf(res, sizeof(res),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Content-Length: %zu\r\n"
        "Connection: keep-alive\r\n\r\n%s",
        strlen(response), response);

    send(client_sock, res, strlen(res), 0);
}

void sendError(int client_sock, const char *message, int error_code) {
    char response[BUFF_SIZE];
    snprintf(response, sizeof(response),
        "HTTP/1.1 %d Error\r\n"
        "Content-Type: text/plain\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Content-Length: %zu\r\n"
        "Connection: keep-alive\r\n\r\n%s",error_code,
        strlen(message), message);

    send(client_sock, response, strlen(response), 0);
}

void send_cookie_response(int client_sock, const char *response, const char *username) {
    char session_id[COOKIE_LENGTH];
    generate_session_id(session_id, COOKIE_LENGTH);

    // Add the session to the store
    add_session(username, session_id);
    // printf("%s\n", username);

    char res[BUFF_SIZE];
    snprintf(res, sizeof(res),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Set-Cookie: session_id=%s; HttpOnly; Path=/\r\n"
        "Content-Length: %zu\r\n"
        "Connection: keep-alive\r\n\r\n%s", session_id,
        strlen(response), response);

    send(client_sock, res, strlen(res), 0);
}

const char *extract_cookie(const char *request, const char *cookie_name) {
    printf("%s\n", request);
    const char *cookie_header = strstr(request, "Cookie: ");
    if (!cookie_header) return NULL;

    cookie_header += strlen("Cookie: ");
    const char *start = strstr(cookie_header, cookie_name);
    if (!start) return NULL;

    start += strlen(cookie_name) + 1; // Skip "cookie_name="
    const char *end = strchr(start, ';');
    size_t length = end ? (size_t)(end - start) : strlen(start);

    char *cookie_value = malloc(length + 1); // Allocate space for null terminator
    strncpy(cookie_value, start, length-1);
    cookie_value[length] = '\0'; // Null terminator
    return cookie_value;
}

int check_cookies(const char *request) {
    const char *session_id = extract_cookie(request, "session_id");
    // printf("%s", session_id);
    
    if (!session_id) {
        return 0;
    }
    const char *username = validate_session(session_id);
    // printf("User: %s\n", username);
    free((void *)session_id);

    if (username && find_user(username) != NULL) {
        return 1;
    }

    return 0;
}

