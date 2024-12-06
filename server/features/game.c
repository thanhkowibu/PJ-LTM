#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <json-c/json.h>

#define TOPIC "database/topic1/topic1.txt"
#define BUFF_SIZE 4096

Question questions[5];
int current_question_index = 0;
ClientProgress client_progress[MAX_PLAYERS];

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

void create_questions() {
    char data[MAX_LINES][MAX_LINE_LENGTH];
    load_data(TOPIC, data);

    srand(time(NULL));
    int indices[6];
    for (int i = 0; i < 6; i++) {
        indices[i] = rand() % MAX_LINES;
    }

    for (int i = 0; i < 5; i++) {
        int idx1 = indices[i];
        int idx2 = indices[i + 1];

        sscanf(data[idx1], "%d|%49[^|]|%lld|%49[^|]|%199[^\n]", 
            &questions[i].id, 
            questions[i].name1, 
            &questions[i].value1, 
            questions[i].unit, 
            questions[i].pic1);

        sscanf(data[idx2], "%d|%49[^|]|%lld|%49[^|]|%199[^\n]", 
            &questions[i].id, 
            questions[i].name2, 
            &questions[i].value2, 
            questions[i].unit, 
            questions[i].pic2);


        printf("Raw line 1: %s\n", data[idx1]);
printf("Raw line 2: %s\n", data[idx2]);
printf("Parsed 1: %d | %s | %lld | %s | %s\n", 
    questions[i].id, questions[i].name1, questions[i].value1, questions[i].unit, questions[i].pic1);
printf("Parsed 2: %d | %s | %lld | %s | %s\n", 
    questions[i].id, questions[i].name2, questions[i].value2, questions[i].unit, questions[i].pic2);


        questions[i].answer = (questions[i].value1 >= questions[i].value2) ? 1 : 2;
        printf("%d\n", questions[i].answer);
    }

    // Initialize client progress
    for (int i = 0; i < MAX_PLAYERS; i++) {
        client_progress[i].user_id = -1; // -1 indicates no user
        client_progress[i].current_question = 0;
        client_progress[i].answered = 0;
        client_progress[i].score = 0;
    }
    current_question_index = 0;
}