#ifndef _VARIABLES_H__
#define _VARIABLES_H__

#include "io_helpers.h"

struct variable {
    char *name;
    char *value;
    struct variable *next;
};

//typedef struct node {
//    struct variable info;
//    struct node *next;
//} Node;


//Header for switching to variable values
void make_var(size_t size, char **tokens);

//Header for insert function
void insert(char *name, char *value);

//Header for search function
char *search(char *str);

//Header for change function
void change(char *str, char *value);

//Header for deallocate function
void deallocate();


#endif