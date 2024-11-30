#include "user.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define ACCOUNT "nguoidung.txt"

typedef struct User {
    char username[50];
    char password[50];
    int status;
    char homepage[50];
    struct User *next;
} User;

User *head = NULL;
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