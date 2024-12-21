#ifndef GAME_H
#define GAME_H

#define MAX_LINES 20
#define MAX_LINE_LENGTH 1000
#define MAX_PLAYERS 3

typedef struct {
    int id;
    char name1[50];
    char name2[50];
    long long int value1;
    long long int value2;
    char unit[50];
    char pic1[200];
    char pic2[200];
    int answer;
} Question;

typedef struct {
    char username[50];
    int current_question;
    int answered;
    int score;
} ClientProgress;

extern Question questions[5];
extern int current_question_index;
extern ClientProgress client_progress[MAX_PLAYERS];

void create_questions();

#endif // GAME_H