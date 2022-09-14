#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include "io_helpers.h"
#include "variables.h"


// ===== Output helpers =====

/* Prereq: str is a NULL terminated string
 */
void display_message(char *str) {
    write(STDOUT_FILENO, str, strnlen(str, MAX_STR_LEN));
}


/* Prereq: pre_str, str are NULL terminated string
 */
void display_error(char *pre_str, char *str) {
    write(STDERR_FILENO, pre_str, strnlen(pre_str, MAX_STR_LEN));
    write(STDERR_FILENO, str, strnlen(str, MAX_STR_LEN));
    write(STDERR_FILENO, "\n", 1);
}


// ===== Input tokenizing =====

/* Prereq: in_ptr points to a character buffer of size > MAX_STR_LEN
 * Return: number of bytes read
 */
ssize_t get_input(char *in_ptr) {
    int retval = read(STDIN_FILENO, in_ptr, MAX_STR_LEN+1);
    int read_len = retval;
    if (retval == -1) {
        read_len = 0;
    }
    if (read_len > MAX_STR_LEN) {
        read_len = 0;
        retval = -1;
        write(STDERR_FILENO, "ERROR: input line too long\n", strlen("ERROR: input line too long\n"));
        int junk = 0;
        while((junk = getchar()) != EOF && junk != '\n');
    }
    in_ptr[read_len] = '\0';
    return retval;
}

/* Prereq: in_ptr is a string, tokens is of size >= len(in_ptr)
 * Warning: in_ptr is modified
 * Return: number of tokens.
 */
size_t tokenize_input(char *in_ptr, char **tokens) {
    char *curr_ptr = strtok(in_ptr, DELIMITERS);
    size_t token_count = 0 ;
    while (curr_ptr != NULL) { 
        tokens[token_count] = curr_ptr;
        token_count += 1;
        curr_ptr = strtok(NULL, DELIMITERS);
    }
    tokens[token_count] = NULL;
    return token_count;
}

/*
The function below works very similary to tokenize_input,
it's just it uses '|' as the delimiter.
*/
int check_for_pipes(char *in_ptr) {
    int length = strlen(in_ptr);
    for (int i = 0; i < length; i++) {
        if (in_ptr[i] == '|') {
            return 1;
        }
    }
    return 0;
}

ssize_t pipe_tokenize(char *in_ptr, char **tokens) {
    char *curr_ptr = strtok(in_ptr, "|");
    size_t process_count = 0 ;
    while (curr_ptr != NULL) { 
        tokens[process_count] = curr_ptr;
        process_count += 1;
        curr_ptr = strtok(NULL, "|");
    }
    tokens[process_count] = NULL;
    return process_count;
}

int need_background(char *input_buf) {
    //Function returns 1 if there is & at the
    //end and 0 if there isn't
    char *excess = strrchr(input_buf, '&');
    if (excess != NULL) {
        int index = excess - input_buf;
        input_buf[index] = '\0';
        return 1;
    }
    return 0;
}
