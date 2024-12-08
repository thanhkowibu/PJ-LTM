#include "user.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "../utils/utils.h"

#define ACCOUNT "database/nguoidung.txt"

User *head = NULL;

__thread User *curUser = NULL; // Thread-local storage for current user

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


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

void setCurUser(User *user) {
    printf("Welcome %s", curUser->username);
    curUser = user;
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

User * find_user(const char * username) {
    pthread_mutex_lock(&mutex);
    User *current = head;
    while (current != NULL) {
        if (strcmp(current->username, username) == 0) {
            pthread_mutex_unlock(&mutex);
            return current;
        }
        current = current->next;
    }
    pthread_mutex_unlock(&mutex);
    return NULL;
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

    // saveUserToFile();

    pthread_mutex_unlock(&mutex);
    return 1; // Registration successful
}

int log_out() {
    curUser = NULL;
    return 1;
}