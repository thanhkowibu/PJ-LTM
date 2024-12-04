#include "room_routes.h"

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

void join_room(int client_sock, const char *request, const char *body) {
        // Locate the JSON body in the HTTP request
    struct json_object *json_request = json_tokener_parse(body);
    struct json_object *room_name_obj;
    struct json_object *username_obj;
    

    const char *room_name = NULL;
    const char *username = NULL;
    if (json_request && json_object_object_get_ex(json_request, "room_name", &room_name_obj) && json_object_object_get_ex(json_request, "username", &username_obj)) {
        room_name = json_object_get_string(room_name_obj);
        username = json_object_get_string(username_obj);
    }

    struct json_object *json_response = json_object_new_object();

    if ((username != NULL) && room_name && add_user_to_room(room_name, username)) {
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
    sendResponse(client_sock, json_object_to_json_string(json_response));

    json_object_put(json_request);
    json_object_put(json_response);
}

void add_room(int client_sock, const char *request, const char *body) {
    struct json_object *json_request = json_tokener_parse(body);
    struct json_object *room_name_obj, *capacity_obj, *username_obj;

    const char *room_name = NULL;
    const char *username = NULL;
    int capacity = 0;
    if (json_request && json_object_object_get_ex(json_request, "room_name", &room_name_obj) &&
        json_object_object_get_ex(json_request, "capacity", &capacity_obj) && json_object_object_get_ex(json_request, "username", &username_obj)) {
        room_name = json_object_get_string(room_name_obj);
        capacity = json_object_get_int(capacity_obj);
        username = json_object_get_string(username_obj);
    }

    struct json_object *json_response = json_object_new_object();
    if (room_name && create_room(room_name, capacity, username)) {
        json_object_object_add(json_response, "status", json_object_new_string("success"));
        json_object_object_add(json_response, "message", json_object_new_string("Room created successfully"));
    } else {
        json_object_object_add(json_response, "status", json_object_new_string("failure"));
        json_object_object_add(json_response, "message", json_object_new_string("Room creation failed"));
    }
    sendResponse(client_sock, json_object_to_json_string(json_response));

    json_object_put(json_request);
    json_object_put(json_response);
}