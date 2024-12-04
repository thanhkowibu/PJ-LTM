#ifndef ROOM_H
#define ROOM_H

#include "user.h"

// cai nay de cho khi xoa ko xoa vao User ma chi xoa trong room
typedef struct UserNode {
    // THong tin cua User co trong phong, co the luu diem so, progess, thoi gian vao phong,...
    User *user;
    struct UserNode *next;
} UserNode;

typedef struct Room {
    // id thay vi ten -> de tim nhung kho xoa hon 1 chut
    char name[50];
    int capacity;
    UserNode *users;
    User * host;
} Room;

int create_room(const char *name, int capacity, const char * curUser);
int delete_room(const char *name);
int add_user_to_room(const char *room_name, const char * curUser);
int delete_user_from_room(const char *room_name, const char * curUser);
int get_user_count_in_room(const char *room_name);
Room* get_room_by_name(const char *name);

#endif // ROOM_H