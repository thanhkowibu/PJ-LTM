#include "cookies.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <json-c/json.h>
#include <stddef.h>

// Session store
Session *session_store = NULL;
pthread_mutex_t session_mutex = PTHREAD_MUTEX_INITIALIZER;

void generate_session_id(char *buffer, size_t length) {
    const char *charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (size_t i = 0; i < length - 1; i++) {
        buffer[i] = charset[rand() % strlen(charset)];
    }
    buffer[length - 1] = '\0';
}

void add_session(const char *username, char *session_id) {
    pthread_mutex_lock(&session_mutex);

    Session *new_session = malloc(sizeof(Session));
    strncpy(new_session->session_id, session_id, COOKIE_LENGTH);
    strncpy(new_session->username, username, sizeof(new_session->username));
    new_session->next = session_store;
    // printf("A: %s\n", new_session->session_id);
    session_store = new_session;

    pthread_mutex_unlock(&session_mutex);
}

const char *validate_session(const char *session_id) {
    pthread_mutex_lock(&session_mutex);

    Session *current = session_store;
    while (current) {
        if (compare_string_sums(current->session_id, session_id) == 0) {
            pthread_mutex_unlock(&session_mutex);
            // printf("C:%s - %s ", current->username, current->session_id);
            return current->username;
        }
        current = current->next;
    }

    pthread_mutex_unlock(&session_mutex);
    return NULL;
}

int sum_ascii(const char *str) {
    int sum = 0;
    while (*str) {
        if ((*str >= 'A' && *str <= 'Z') || (*str >= 'a' && *str <= 'z') || (*str >= '0' && *str <= '9')) {
            sum += (unsigned char)(*str);
        }
        str++;
    }
    return sum;
}

int compare_string_sums(const char *str1, const char *str2) {
    int sum1 = sum_ascii(str1);
    int sum2 = sum_ascii(str2);
    return sum1 - sum2;
}


void delete_session(const char *session_id) {
    pthread_mutex_lock(&session_mutex);

    Session **current = &session_store;
    while (*current) {
        if (strcmp((*current)->session_id, session_id) == 0) {
            Session *to_delete = *current;
            *current = (*current)->next;
            free(to_delete);
            break;
        }
        current = &(*current)->next;
    }

    pthread_mutex_unlock(&session_mutex);
}
