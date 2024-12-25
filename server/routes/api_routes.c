#include "api_routes.h"
#include "../core/sse.h"
#include "../features/game.h"
#include "../utils/utils.h"
#include "../middleware/cookies.h"
#include "../features/room.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <json-c/json.h>
#include <time.h>

extern Client clients[MAX_CLIENTS];
extern fd_set master_set;

void initialize_game(int client_sock, const char *request, const char *body) {
    printf("called initialize_game");
    struct json_object *json_request = json_tokener_parse(body);
    struct json_object *room_name_obj, *num_players_obj;
    const char *room_name = NULL;
    int num_players = 0;

    if (json_request && json_object_object_get_ex(json_request, "room_name", &room_name_obj) &&
        json_object_object_get_ex(json_request, "num_players", &num_players_obj)) {
        room_name = json_object_get_string(room_name_obj);
        num_players = json_object_get_int(num_players_obj);
    } else {
        sendError(client_sock, "Invalid request", 400);
        return;
    }
    // Delete the room if it already exists
    if (get_room_by_name(room_name) != NULL) {
        if (!delete_room(room_name)) {
            sendError(client_sock, "Failed to delete existing room", 500);
            return;
        }
    } else {
        sendError(client_sock, "Waiting room not found", 500);
        return;
    }
    GameRoom *room = find_or_create_room(room_name);
    if (!room) {
        sendError(client_sock, "Server is full", 500);
        return;
    }

    room->num_players = num_players;
    room->question_start_time = time(NULL); // Set the start time for the first question

    sendResponse(client_sock, "{\"status\":\"Game initialized\"}");
    // Create the broadcast JSON object
    struct json_object *broadcast_json = json_object_new_object();
    json_object_object_add(broadcast_json, "action", json_object_new_string("start"));
    json_object_object_add(broadcast_json, "room_name", json_object_new_string(room_name));

    // Broadcast the JSON object
    broadcast_json_object(broadcast_json, client_sock);

    json_object_put(broadcast_json);
}

void get_game_data(int client_sock, const char *request, const char *body) {
    printf("get_game_data called\n");
    struct json_object *json_request = json_tokener_parse(body);
    struct json_object *room_name_obj;

    const char *room_name = NULL;
    const char *username = NULL;

    if (check_cookies(request)) {
        const char *session_id = extract_cookie(request, "session_id");
        // printf("Session: %s\n", session_id);
        username = validate_session(session_id);
        // printf("User: %s\n", username);
    }

    if (json_request && json_object_object_get_ex(json_request, "room_name", &room_name_obj) && username) {
        room_name = json_object_get_string(room_name_obj);
        // printf("Room name: %s\n", room_name);
    } else {
        sendError(client_sock, "Invalid request", 400);
        return;
    }

    GameRoom *room = find_or_create_room(room_name);
    if (!room) {
        sendError(client_sock, "Room not found", 404);
        return;
    }

    int client_index = -1;
    for (int i = 0; i < room->num_players; i++) {
        if (strcmp(room->client_progress[i].username, username) == 0) {
            client_index = i;
            break;
        }
    }

    if (client_index == -1) {
        for (int i = 0; i < room->num_players; i++) {
            if (room->client_progress[i].username[0] == '\0') {
                strncpy(room->client_progress[i].username, username, 50);
                printf("New user assigned: %s\n", room->client_progress[i].username);
                client_index = i;
                break;
            }
        }
    }
    if (client_index == -1) {
        sendError(client_sock, "Server is full", 500);
        return;
    }

    int question_index = room->client_progress[client_index].current_question;

    if (question_index < 0 || question_index >= 5) {
        sendError(client_sock, "Invalid question index", 500);
        return;
    }

    struct json_object *json_response = json_object_new_object();
    json_object_object_add(json_response, "id", json_object_new_int(room->questions[question_index].id));
    json_object_object_add(json_response, "name1", json_object_new_string(room->questions[question_index].name1));
    json_object_object_add(json_response, "name2", json_object_new_string(room->questions[question_index].name2));
    json_object_object_add(json_response, "pic1", json_object_new_string(room->questions[question_index].pic1));
    json_object_object_add(json_response, "pic2", json_object_new_string(room->questions[question_index].pic2));
    json_object_object_add(json_response, "unit", json_object_new_string(room->questions[question_index].unit));
    json_object_object_add(json_response, "timestamp", json_object_new_int(room->question_start_time)); // Add timestamp

    sendResponse(client_sock, json_object_to_json_string(json_response));
    json_object_put(json_response);
}

void handle_choice(int client_sock, const char *request, const char *body) {
    struct json_object *json_request = json_tokener_parse(body);
    struct json_object *choice_obj;
    struct json_object *room_name_obj;
    struct json_object *remaining_time_obj;
    int choice = 0;
    int remaining_time = 0;

    if (json_request && json_object_object_get_ex(json_request, "choice", &choice_obj)) {
        choice = json_object_get_int(choice_obj);
    }

    if (json_request && json_object_object_get_ex(json_request, "remaining_time", &remaining_time_obj)) {
        remaining_time = json_object_get_int(remaining_time_obj);
    }

    const char *room_name = NULL;
    const char *username = NULL;

    if (check_cookies(request)) {
        const char *session_id = extract_cookie(request, "session_id");
        username = validate_session(session_id);
        // printf("%s", username);
    }

    if (json_request && json_object_object_get_ex(json_request, "room_name", &room_name_obj) && username) {
        room_name = json_object_get_string(room_name_obj);
    } else {
        sendError(client_sock, "Invalid request", 400);
        return;
    }

    GameRoom *room = find_or_create_room(room_name);
    if (!room) {
        sendError(client_sock, "Room not found", 404);
        return;
    }

    int client_index = -1;
    for (int i = 0; i < room->num_players; i++) {
        if (strcmp(room->client_progress[i].username, username) == 0) {
            client_index = i;
            break;
        }
    }

    if (client_index == -1) {
        sendError(client_sock, "User not found", 500);
        return;
    }

    int question_index = room->client_progress[client_index].current_question;
    int base_score = (choice == room->questions[question_index].answer) ? 1000 : 0;
    if (base_score == 1000) {
        room->client_progress[client_index].streak++;
    } else {
        room->client_progress[client_index].streak = 0;
    }
    int bonus = (base_score == 1000) ? remaining_time * 10 * room->client_progress[client_index].streak : 0;
    int total_score = base_score + bonus;

    room->client_progress[client_index].answered = 1;
    room->client_progress[client_index].score += total_score;

    // Create the broadcast JSON object
    struct json_object *broadcast_json = json_object_new_object();
    json_object_object_add(broadcast_json, "action", json_object_new_string("an user answered"));
    json_object_object_add(broadcast_json, "room_name", json_object_new_string(room_name));

    // Broadcast the JSON object
    broadcast_json_object(broadcast_json, client_sock);

    json_object_put(broadcast_json);

    // Check if all clients have answered
    printf("---------\n");
    for (int i = 0; i < room->num_players; i++) {
        printf("%s:  %d\n", room->client_progress[i].username, room->client_progress[i].answered);
    }
    printf("current: %d\n", room->current_question_index);
    printf("---------\n");

    int all_answered = 1;
    for (int i = 0; i < room->num_players; i++) {
        if (room->client_progress[i].username[0] != '\0' && !room->client_progress[i].answered) {
            all_answered = 0;
            break;
        }
    }

    if (all_answered) {
        room->all_answered = 1;
        room->all_answered_time = time(NULL);
    }

    struct json_object *json_response = json_object_new_object();
    json_object_object_add(json_response, "base_score", json_object_new_int(base_score));
    json_object_object_add(json_response, "bonus", json_object_new_int(bonus));
    json_object_object_add(json_response, "total_score", json_object_new_int(total_score));
    json_object_object_add(json_response, "value1", json_object_new_int(room->questions[question_index].value1));
    json_object_object_add(json_response, "value2", json_object_new_int(room->questions[question_index].value2));
    json_object_object_add(json_response, "remaining_time", json_object_new_int(remaining_time));
    json_object_object_add(json_response, "streak", json_object_new_int(room->client_progress[client_index].streak));

    sendResponse(client_sock, json_object_to_json_string(json_response));

    json_object_put(json_request);
    json_object_put(json_response);
}

void get_game_result(int client_sock, const char *request, const char *body) {
    struct json_object *json_request = json_tokener_parse(body);
    struct json_object *room_name_obj;
    const char *room_name = NULL;
    if (json_request && json_object_object_get_ex(json_request, "room_name", &room_name_obj)) {
        room_name = json_object_get_string(room_name_obj);
    } else {
        sendError(client_sock, "Invalid request", 400);
        return;
    }

    GameRoom *room = find_or_create_room(room_name);
    if (!room) {
        sendError(client_sock, "Room not found", 404);
        return;
    }

    struct json_object *json_response = json_object_new_array();

    for (int i = 0; i < room->num_players; i++) {
        if (room->client_progress[i].username[0] != '\0') {
            struct json_object *json_player = json_object_new_object();
            json_object_object_add(json_player, "username", json_object_new_string(room->client_progress[i].username));
            json_object_object_add(json_player, "score", json_object_new_int(room->client_progress[i].score));
            json_object_array_add(json_response, json_player);
        }
    }

    sendResponse(client_sock, json_object_to_json_string(json_response));
    json_object_put(json_response);
}

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