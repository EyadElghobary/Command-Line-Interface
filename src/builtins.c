#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include<sys/wait.h>
#include <signal.h>

#include "builtins.h"
#include "io_helpers.h"
#include "variables.h"
#include <dirent.h>
#include "other_helpers.h"
#include "commands.h"


int server_num = -1;

// ====== Command execution =====

/* Return: index of builtin or -1 if cmd doesn't match a builtin
 */
bn_ptr check_builtin(const char *cmd) {
    ssize_t cmd_num = 0;
    while (cmd_num < BUILTINS_COUNT &&
           strncmp(BUILTINS[cmd_num], cmd, MAX_STR_LEN) != 0) {
        cmd_num += 1;
    }
    return BUILTINS_FN[cmd_num];
}


// ===== Builtins =====

/* Prereq: tokens is a NULL terminated sequence of strings.
 * Return 0 on success and -1 on error ... but there are no errors on echo. 
 */
ssize_t bn_echo(char **tokens) {
    ssize_t index = 1;

    if (tokens[index] != NULL) {
        display_message(tokens[index]);
        index++; 
    }

    while (tokens[index] != NULL) {
        display_message(" ");
        display_message(tokens[index]);
        index++;
    }
    display_message("\n");
    return 0;
}

ssize_t bn_ls(char **tokens) {
    int size = get_size();
    char path[MAX_STR_LEN] = {'\0'};
    int rec = 0;
    int d_arg = -1;
    char f_arg[MAX_STR_LEN] = {'\0'};

    if (size == 1) {
        return ls_helper(".", "", d_arg);
    }

    if (size == 2) {
        return ls_helper(tokens[1], "", d_arg);
    }

    int i = 1;
    while (i < size) {
        if (strncmp(tokens[i], "--rec", strlen("--rec")) == 0) {
            rec = 1;
            strncpy(path, tokens[i+1], strlen(tokens[i+1]));
            i++;
        } else if (strncmp(tokens[i], "--d", strlen("--d")) == 0) {
            d_arg = strtol(tokens[i + 1], NULL, 10);
            i++;
        } else if (strncmp(tokens[i], "--f", strlen("--f")) == 0) {
            strncpy(f_arg, tokens[i + 1], strlen(tokens[i+1]));
            i++;
        } else {
            if (strlen(path) == 0) {
                strncpy(path, tokens[i], strlen(tokens[i]) + 1);
            }
        }
        i++;
    }

    if ((d_arg == -1 && rec != 0) || (d_arg != -1 && rec == 0)) {
        display_error("", "ERROR: --rec and --d have to be provided together.");
        return -1;
    }

    if (strlen(path) == 0) {
        return ls_helper(".", f_arg, d_arg);
    } else  {
        return ls_helper(path, f_arg, d_arg); 
    }
    return 0;
}

ssize_t bn_cd(char **tokens) {
    int size = get_size();
    int dot_count = 0;

    if (size == 1) {
        chdir("/"); 
        return 0;
    }

    if (size != 2) {
        display_error("ERROR: Invalid path", "");
        return -1;
    }

    dot_count = cd_helper(tokens[1]);

    if (dot_count > 0) {
        for (int i = 1; i < dot_count; i++) {
            chdir("..");
        }
        return 0;
    }

    if (chdir(tokens[1]) == -1) {
        display_error("ERROR: Invalid path", "");
        return -1;
    }
    return 0;
}

ssize_t bn_cat(char **tokens) {
    int size = get_size();
    char line[MAX_STR_LEN + 1];
    if (size == 1) {
        /
        if (fgets(line, MAX_STR_LEN + 1, stdin) == NULL) {
            display_error("", "ERROR: No input source provided");
            return -1;
        }
        display_message(line);
        while (fgets(line, MAX_STR_LEN + 1, stdin) != NULL) {
            display_message(line);
        }
        return 0;
    }

    FILE *f = fopen(tokens[1], "r");
    if (f == NULL) {
        display_error("ERROR: Cannot open file", "");
        return -1;
    }

    while (fgets(line, MAX_STR_LEN + 1, f) != NULL) {
        display_message(line);
    }
    return 0;
}

ssize_t bn_wc(char **tokens) {
    int size = get_size();
    int line_count = 0;
    int word_count = 0;
    int char_count = 0;
    int curr = 0;
    char s[MAX_STR_LEN];
    
    int c;
    if (size == 1) {
        c = fgetc(stdin);
        while (c != EOF) {
            if (c == ' ' || c == '\n' || c == '\t' || c == '\r'){
                if (c == '\n'){
                    line_count += 1;
                }
            curr = 0;
            } else if (curr == 0){
                curr = 1;
                word_count += 1;
            }
            char_count +=1;
            c = fgetc(stdin);
        }
    } else {
        FILE *f = fopen(tokens[1], "r");

        if (f == NULL) {
            display_error("ERROR: Cannot open file", "");
            return -1;
        }

        c = fgetc(f);
        while (c != EOF) {
            if (c == ' ' || c == '\n' || c == '\t' || c == '\r'){
                if (c == '\n'){
                    line_count += 1;
                }
                curr = 0;
            } else if (curr == 0) {
                curr = 1;
                word_count += 1;
            }
            char_count +=1;
            c = fgetc(f);
        }
        fclose(f);
    }

    display_message("word count ");
    sprintf(s, "%d", word_count);
    display_message(s);
    display_message("\n");
    display_message("character count ");
    sprintf(s, "%d", char_count);
    display_message(s);
    display_message("\n");
    display_message("newline count ");
    sprintf(s, "%d", line_count);
    display_message(s);
    display_message("\n");
    return 0;
}

ssize_t execute_command(char **tokens) {
    int token_count = get_size();
    //In this function if -1 is returned then we have an error else 0 is returned.
    int signal_num = SIGTERM;
    int pid;
    bn_ptr builtin_fn = check_builtin(tokens[0]);
    if (builtin_fn != NULL) {
        ssize_t err = builtin_fn(tokens);
        if (err == - 1) {
            return -1;
        }
    } else if (strncmp(tokens[0], "start-server", 13) == 0) {
        if (server_num != -1) {
            display_error("", "Can't have more than one server");
            return -1;
        }
        int n = fork();
        if (n < 0) {
            display_error("", "fork");
            return -1;
        } else if (n == 0) {
            int ret = startserver(tokens);
            if (ret == -1) {
                display_error("ERROR: Builtin failed: ", tokens[0]);
            }
            exit(ret);
        }
        int status;
        int exitstatus = WEXITSTATUS(status);
        server_num = n; 
        return exitstatus;

    } else if (strncmp(tokens[0], "close-server", 13) == 0) {
        if (server_num != -1) {
            kill(server_num, SIGTERM);
            server_num = -1;
        }
    } else if (strncmp(tokens[0], "start-client", 13) == 0) {
        if (token_count != 3) {
            display_error("", "ERROR: Not enough arguments");
            return -1;
        }
        return start_client(tokens);
    } else if (strncmp(tokens[0], "send", 5) == 0) {
        return send_command(tokens, token_count);
    } else if (strncmp(tokens[0], "kill", 5) == 0) {
        if (token_count != 2 && token_count != 3) {
            display_error("", "kill error");
            return -1;
        }
        if (token_count == 3) {
            signal_num = atoi(tokens[2]);
        }
        pid = atoi(tokens[1]);
        return kill_command(pid, signal_num);
    } else if (strncmp(tokens[0], "ps", 3) == 0) {
        ps_command();
    } else {
        int exec_n = fork();
        if (exec_n < 0) {
            display_error("", "fork error");
            exit(1);
        } else if (exec_n == 0) {
            if ((execvp(tokens[0], tokens)) == -1) {
                display_error("ERROR: Unrecognized command: ", tokens[0]);
                exit(2);
            }
            exit(0);
        }
        int status;
        wait(&status);
        int exitstatus = WEXITSTATUS(status);
        if (exitstatus == 0) {
            return 0;
        } else if (exitstatus == 1) {
            return -1;
        } else {
            return -2;
        }
    }
    return 0;
}
