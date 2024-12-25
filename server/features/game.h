#ifndef GAME_H
#define GAME_H

#define MAX_LINES 100
#define MAX_LINE_LENGTH 256
#define MAX_PLAYERS 3
#define MAX_ROOMS 10
#define MAX_POWERUPS 4
#include <time.h>

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
    int streak;
    int used_powerup[MAX_POWERUPS]; // Array to track used power-ups
} ClientProgress;

typedef struct {
    char room_name[50];
    Question questions[5];
    int current_question_index;
    ClientProgress client_progress[MAX_PLAYERS];
    int num_players;
    time_t question_start_time;
    int all_answered; // Flag to indicate if all players have answered
    time_t all_answered_time; // Timestamp when all players have answered
} GameRoom;

extern GameRoom game_rooms[MAX_ROOMS];
extern int num_rooms;

void create_questions(GameRoom *room);
GameRoom* find_or_create_room(const char *room_name);
void delete_game_room(const char *room_name);
void check_timeout(GameRoom *room);

#endif // GAME_H