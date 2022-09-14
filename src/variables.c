#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "variables.h"
#include "io_helpers.h"


struct variable *front = NULL;

void make_var(size_t size, char **tokens) {
    for (int index = 0; index < size; index++) {
        if (tokens[index][0] == '$') {
            char *result = search(tokens[index] + 1);
            if (strlen(result) > 0) {
                tokens[index] = result;
            }
        }
    }
}

void insert(char *name, char *value) {
    struct variable *new_node = malloc(sizeof(struct variable));
    new_node->name = malloc(MAX_STR_LEN + 1);
    new_node->value = malloc(MAX_STR_LEN + 1);
    strcpy(new_node->name, name);
    strcpy(new_node->value, value);
    new_node->next = front;
    front = new_node;
}


char *search(char *str) {
    //Loop through your linked list to find the value stored in the variable or NULL if variable not found.
    struct variable *curr = front;
    while (curr != NULL && strcmp(str, curr->name) != 0) {
        curr = curr->next;
    }
    if (curr == NULL) {
        return "";
    }
    return curr->value;
}

void change(char *str, char *value) {
    struct variable *curr = front;
    while (strcmp(str, curr->name) != 0) {
        curr = curr->next;
    }
    strncpy(curr->value, value, strlen(value) + 1);
}


void deallocate() {
    //Loop through your linked list, freeing up all the nodes.
    struct variable *curr = front;
    struct variable *temp;
    while (curr != NULL) {
        temp = curr;
        curr = curr->next;
        free(temp->name);
        free(temp->value);
        free(temp);
    }
}