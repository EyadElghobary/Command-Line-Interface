#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#include "io_helpers.h"

int size = 0;

void set_size(int token_count) {
    size = token_count;
}

int get_size() {
    return size;
}

int cd_helper(char *str) {
    int string_size = strlen(str);
    char *temp = str;
    char *ptr = strchr(str, '.');
    int count = 0;

    while (ptr != NULL) {
        temp = ptr;
        temp++;
        ptr = strchr(temp, '.');
        count++;
    }

    if (count != string_size) {
        count = 0;
    }
    return count;
}

int ls_helper(char *path, char *f_arg, int d_arg) {
    DIR *d = opendir(path);
    char final_path[MAX_STR_LEN];
    struct dirent *d_ptr;

    if (d == NULL) {
        closedir(d);
        display_error("", "ERROR: Invalid path");
        return -1;
    }
    if (d_arg <= 1) {
        d_ptr = readdir(d);
        while (d_ptr != NULL) {
            if (strstr(d_ptr->d_name, f_arg) != NULL) {
                display_message(d_ptr->d_name);
                display_message("\n"); 
            }
            d_ptr = readdir(d);
        }
        closedir(d);
        return 0;
    }
    d_ptr = readdir(d);
    while (d_ptr != NULL) {
        if (strstr(d_ptr->d_name, f_arg) != NULL) {
                display_message(d_ptr->d_name);
                display_message("\n");
        }
        if (d_ptr->d_type == DT_DIR) {
            //This is for a file
            final_path[0] = '\0';
            strncat(final_path, path, strlen(path));
            strcat(final_path, "/");
            if (cd_helper(d_ptr->d_name) == 0) {
                //This is for a directory
                strncat(final_path, d_ptr->d_name, strlen(d_ptr->d_name));
                ls_helper(final_path, f_arg, d_arg - 1);
            }
        }
        d_ptr = readdir(d);
    }
    closedir(d);
    return 0;
}

int pipe_count(char **tokens) {
    //Returns  the number of pipes present
    int count = 0;

    for (int i = 0; i < size; i++) {
        if (strchr(tokens[i], '|') != NULL) {
            count++;
        }
    }
    return count;
}