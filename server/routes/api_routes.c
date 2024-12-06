#include "api_routes.h"
#include "../features/game.h"
#include "../utils/utils.h"

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <json-c/json.h>
#include "sse.h"
#include <time.h>

extern Client clients[MAX_CLIENTS];
extern fd_set master_set;

void initialize_game(int client_sock, const char *request, const char *body) {
    create_questions();
    sendResponse(client_sock, "{\"status\":\"Game initialized\"}");
}

void get_game_data(int client_sock, const char *request, const char *body) {
    // Find the client progress
    int user_id = get_user_id_from_request(request); // Implement this function to extract user_id from request
    // if (user_id == -1) {
    //     sendError(client_sock, "Invalid User-ID", 400);
    //     return;
    // }
    int client_index = -1;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_progress[i].user_id == user_id) {
            client_index = i;
            break;
        }
    }

    if (client_index == -1) {
        // New client, assign a slot
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_progress[i].user_id == -1) {
                client_progress[i].user_id = user_id;
                client_index = i;
                break;
            }
        }
    }

    if (client_index == -1) {
        sendError(client_sock, "Server is full", 500);
        return;
    }

    int question_index = client_progress[client_index].current_question;

    if (question_index < 0 || question_index >= 5) {
        sendError(client_sock, "Invalid question index", 500);
        return;
    }

    struct json_object *json_response = json_object_new_object();
    json_object_object_add(json_response, "id", json_object_new_int(questions[question_index].id));
    json_object_object_add(json_response, "name1", json_object_new_string(questions[question_index].name1));
    json_object_object_add(json_response, "name2", json_object_new_string(questions[question_index].name2));
    json_object_object_add(json_response, "pic1", json_object_new_string(questions[question_index].pic1));
    json_object_object_add(json_response, "pic2", json_object_new_string(questions[question_index].pic2));
    json_object_object_add(json_response, "unit", json_object_new_string(questions[question_index].unit));

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

void handle_choice(int client_sock, const char *request, const char *body) {
    struct json_object *json_request = json_tokener_parse(body);
    struct json_object *choice_obj;
    int choice = 0;

    if (json_request && json_object_object_get_ex(json_request, "choice", &choice_obj)) {
        choice = json_object_get_int(choice_obj);
    }

    int user_id = get_user_id_from_request(request); // Implement this function to extract user_id from request
    int client_index = -1;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_progress[i].user_id == user_id) {
            client_index = i;
            break;
        }
    }

    if (client_index == -1) {
        sendError(client_sock, "User not found", 500);
        return;
    }

    int question_index = client_progress[client_index].current_question;
    int score = (choice == questions[question_index].answer) ? 1 : 0;

    client_progress[client_index].answered = 1;
    client_progress[client_index].score += score;
    broadcast_message("An user answered", client_sock);

    // Check if all clients have answered
    printf("---------\n");
    for (int i = 0; i < MAX_PLAYERS; i++) {
        printf("%d:  %d\n",client_progress[i].user_id,client_progress[i].answered);
    }
    printf("current: %d\n",current_question_index);
    printf("---------\n");

    int all_answered = 1;
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (client_progress[i].user_id != -1 && !client_progress[i].answered) {
            all_answered = 0;
            break;
        }
    }

    if (all_answered) {
        // Move to the next question
        current_question_index++;
        if (current_question_index >= 5) {
            // If all questions are answered, broadcast "Finish"
            broadcast_message("Finish", client_sock);
        } else {
            for (int i = 0; i < MAX_PLAYERS; i++) {
                if (client_progress[i].user_id != -1) {
                    client_progress[i].current_question = current_question_index;
                    client_progress[i].answered = 0;
                }
            }
            // Notify clients to fetch the next question
            broadcast_message("Next", client_sock);
        }
    }

    struct json_object *json_response = json_object_new_object();
    json_object_object_add(json_response, "score", json_object_new_int(score));
    json_object_object_add(json_response, "value1", json_object_new_int(questions[question_index].value1));
    json_object_object_add(json_response, "value2", json_object_new_int(questions[question_index].value2));

    sendResponse(client_sock, json_object_to_json_string(json_response));

    json_object_put(json_request);
    json_object_put(json_response);
}

void get_game_result(int client_sock, const char *request, const char *body) {
    struct json_object *json_response = json_object_new_array();

    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (client_progress[i].user_id != -1) {
            struct json_object *json_player = json_object_new_object();
            json_object_object_add(json_player, "user_id", json_object_new_int(client_progress[i].user_id));
            json_object_object_add(json_player, "score", json_object_new_int(client_progress[i].score));
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