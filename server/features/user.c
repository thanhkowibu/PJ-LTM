#include "user.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define ACCOUNT "data/nguoidung.txt"

User *head = NULL;


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

__thread User *curUser = NULL; 

void loadUserFromFile() {
    pthread_mutex_lock(&mutex);

    FILE* fileHandle = fopen(ACCOUNT, "r");
    if(!fileHandle){
        printf("Unable to open the file: %s\n", ACCOUNT);
        pthread_mutex_unlock(&mutex);
        return;
    }
    char tempLine[150];
    while(fgets(tempLine, sizeof(tempLine), fileHandle)){
        User *tempUser = malloc(sizeof(User));
        sscanf(tempLine, "%s %s %d %s", tempUser->username, tempUser->password, &tempUser->status, tempUser->homepage);
        tempUser->next = head;
        head = tempUser;
    }
    fclose(fileHandle);

    pthread_mutex_unlock(&mutex);
}

void saveUserToFile() {
    pthread_mutex_lock(&mutex);

    FILE* output = fopen(ACCOUNT, "w");
    if(output == NULL){
        printf("Failed to open %s for writing.\n", ACCOUNT);
        pthread_mutex_unlock(&mutex);
        return;
    }

    for(User* currentUser = head; currentUser != NULL; currentUser = currentUser->next){
        fprintf(output, "%s %s %d %s\n", currentUser->username, currentUser->password, currentUser->status, currentUser->homepage);
    }
    fclose(output);

    pthread_mutex_unlock(&mutex);
}


User * getCurUser() {
    return curUser;
}

int setCurUser(User *user) {
    curUser = user;
    return 1;
}

int requiredLogin() {
    return (curUser != NULL);
}

// Get info of cur User (will update when user have more info)
int getCurrentUserInfo(char *username) {
    if (curUser == NULL) {
        return 0; // No current user
    }
    strcpy(username, curUser->username);
    return 1; // Successfully retrieved current user info
}

// Set info of cur User (will update when user have more info)
int setCurrentUserInfo (char *username) {
    if(curUser == NULL){
        return 0;
    }
    strcpy(curUser->username, username);
    return 1;
}

int authenticate_user(const char *username, const char *password) {
    pthread_mutex_lock(&mutex);
    User *current = head;
    while (current != NULL) {
        if (strcmp(current->username, username) == 0 && strcmp(current->password, password) == 0) {
            pthread_mutex_unlock(&mutex);
            curUser = current;
            return 1; // Authentication successful
        }
        current = current->next;
    }
    pthread_mutex_unlock(&mutex);
    return 0; // Authentication failed
}

int register_user(const char *username, const char *password) {
    pthread_mutex_lock(&mutex);
    User *current = head;
    while (current != NULL) {
        if (strcmp(current->username, username) == 0) {
            pthread_mutex_unlock(&mutex);
            return 0; // User already exists
        }
        current = current->next;
    }

    User *new_user = malloc(sizeof(User));
    if (new_user == NULL) {
        pthread_mutex_unlock(&mutex);
        return 0; // Memory allocation failed
    }
    strcpy(new_user->username, username);
    strcpy(new_user->password, password);
    new_user->status = 1; // Default status
    strcpy(new_user->homepage, ""); // Default homepage
    new_user->next = head;
    head = new_user;

    pthread_mutex_unlock(&mutex);
    return 1; // Registration successful
}

int log_out() {
    if(curUser == NULL){
        return 0;
    }
    curUser = NULL;
    return 1;
}