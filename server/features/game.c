#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <json-c/json.h>

#define TOPIC "database/topic1/topic1.txt"
#define BUFF_SIZE 4096

GameRoom game_rooms[MAX_ROOMS];
int num_rooms = 0;

void load_data(const char *filename, char data[MAX_LINES][MAX_LINE_LENGTH]) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }

    int i = 0;
    while (fgets(data[i], MAX_LINE_LENGTH, file) && i < MAX_LINES) {
        data[i][strcspn(data[i], "\n")] = '\0'; // Remove newline character
        i++;
    }

    fclose(file);
}

void shuffle(int *array, size_t n) {
    if (n > 1) {
        size_t i;
        for (i = 0; i < n - 1; i++) {
            size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
            int t = array[j];
            array[j] = array[i];
            array[i] = t;
        }
    }
}

void create_questions(GameRoom *room) {
    char data[MAX_LINES][MAX_LINE_LENGTH];
    load_data(TOPIC, data);

    srand(time(NULL));
    int indices[MAX_LINES];
    for (int i = 0; i < MAX_LINES; i++) {
        indices[i] = i;
    }

    shuffle(indices, MAX_LINES);

    // Pick the first 6 distinct entries
    int selected_indices[6];
    for (int i = 0; i < 6; i++) {
        selected_indices[i] = indices[i];
    }

    // Create 5 questions from the 6 entries
    for (int i = 0; i < 5; i++) {
        int idx1 = selected_indices[i];
        int idx2 = selected_indices[(i + 1) % 6];

        if (i % 2 == 1) {
            // Swap the order for odd indices
            sscanf(data[idx2], "%d|%49[^|]|%lld|%49[^|]|%199[^\n]", 
                &room->questions[i].id, 
                room->questions[i].name1, 
                &room->questions[i].value1, 
                room->questions[i].unit, 
                room->questions[i].pic1);

            sscanf(data[idx1], "%d|%49[^|]|%lld|%49[^|]|%199[^\n]", 
                &room->questions[i].id, 
                room->questions[i].name2, 
                &room->questions[i].value2, 
                room->questions[i].unit, 
                room->questions[i].pic2);
        } else {
            // Default order for even indices
            sscanf(data[idx1], "%d|%49[^|]|%lld|%49[^|]|%199[^\n]", 
                &room->questions[i].id, 
                room->questions[i].name1, 
                &room->questions[i].value1, 
                room->questions[i].unit, 
                room->questions[i].pic1);

            sscanf(data[idx2], "%d|%49[^|]|%lld|%49[^|]|%199[^\n]", 
                &room->questions[i].id, 
                room->questions[i].name2, 
                &room->questions[i].value2, 
                room->questions[i].unit, 
                room->questions[i].pic2);
        }

        printf("Raw line 1: %s\n", data[idx1]);
        printf("Raw line 2: %s\n", data[idx2]);
        printf("Parsed 1: %d | %s | %lld | %s | %s\n", 
        room->questions[i].id, room->questions[i].name1, room->questions[i].value1, room->questions[i].unit, room->questions[i].pic1);
        printf("Parsed 2: %d | %s | %lld | %s | %s\n", 
        room->questions[i].id, room->questions[i].name2, room->questions[i].value2, room->questions[i].unit, room->questions[i].pic2);

        room->questions[i].answer = (room->questions[i].value1 >= room->questions[i].value2) ? 1 : 2;
        printf("%d\n", room->questions[i].answer);
    }

    // Initialize client progress
    for (int i = 0; i < MAX_PLAYERS; i++) {
        room->client_progress[i].username[0] = '\0'; // Empty string indicates no user
        room->client_progress[i].current_question = 0;
        room->client_progress[i].answered = 0;
        room->client_progress[i].score = 0;
    }
    room->current_question_index = 0;
}

GameRoom* find_or_create_room(const char *room_name) {
    for (int i = 0; i < num_rooms; i++) {
        if (strcmp(game_rooms[i].room_name, room_name) == 0) {
            return &game_rooms[i];
        }
    }

    if (num_rooms < MAX_ROOMS) {
        GameRoom *new_room = &game_rooms[num_rooms++];
        strncpy(new_room->room_name, room_name, sizeof(new_room->room_name));
        create_questions(new_room);
        return new_room;
    }

    return NULL; // No available room slots
}