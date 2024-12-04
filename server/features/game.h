#ifndef GAME_H
#define GAME_H

#define MAX_LINES 20
#define MAX_LINE_LENGTH 256
#define MAX_PLAYERS 3

typedef struct {
    int id;
    char name1[50];
    char name2[50];
    char pic1[50];
    char pic2[50];
    int value1;
    int value2;
    char unit[50];
    int answer;
} Question;

typedef struct {
    int user_id;
    int current_question;
    int answered;
    int score;
} ClientProgress;

extern Question questions[5];
extern int current_question_index;
extern ClientProgress client_progress[MAX_PLAYERS];

void create_questions();

#endif // GAME_H