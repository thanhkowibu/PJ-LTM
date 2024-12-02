#ifndef USER_H
#define USER_H

typedef struct User {
    char username[50];
    char password[50];
    int status;
    char homepage[50];
    struct User *next;
} User;

void loadUserFromFile();
void saveUserToFile();

int authenticate_user(const char *username, const char *password);
int register_user(const char *username, const char *password);
int log_out();
int getCurrentUserInfo(char *username);
int requiredLogin();

int setCurUser(User *user);

User *getCurUser();

#endif // USER_H