#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include<sys/wait.h>
#include <signal.h>

#include "builtins.h"
#include "io_helpers.h"
#include "variables.h"
#include "other_helpers.h"
#include "commands.h"


void handler(int signal) {
    display_message("\n");
}

int main(int argc, char* argv[]) {
    char *prompt = "mysh$ "; 

    char input_buf[MAX_STR_LEN + 1];
    input_buf[MAX_STR_LEN] = '\0';
    char *token_arr[MAX_STR_LEN] = {NULL};
    char *temp_arr[MAX_STR_LEN] = {NULL};
    size_t token_count;

    ssize_t error;

    char command[MAX_STR_LEN + 1];
    int original_stdout = dup(STDOUT_FILENO);
    int original_stdin = dup(STDIN_FILENO);

    int background;
    char buffer [100];

    struct sigaction terminate_act;
    terminate_act.sa_handler = handler;
    terminate_act.sa_flags = 0;
    sigemptyset(&terminate_act.sa_mask);
    sigaction(SIGINT, &terminate_act, NULL);


    while (1) {
        check_all_process();
        display_message(prompt);
        int ret = get_input(input_buf);
        background = need_background(input_buf);
        if (background == 1) {
            //continue;
            strncpy(command, input_buf, strlen(input_buf) + 1);
            token_count = tokenize_input(input_buf, token_arr);
            set_size(token_count);
            int n = fork();
            if (n < 0) {
                display_error("", "fork error");
                exit(1);
            } else if (n == 0) {
               
                error = execute_command(token_arr);
                if (error == -1) {
                    display_error("ERROR: Builtin failed: ", token_arr[0]);
                    //exit(1);
                }
                exit(0);
            }
            
            add_proc(n, token_arr[0], command);
            int process_num = get_proc_num();
            snprintf(buffer, 100, "[%d] %d\n", process_num, n);
            display_message(buffer);
            continue;
        }
        int pipes = check_for_pipes(input_buf);
        /* Checking if we have any '|' in our code */
        
        if (pipes == 1) {
            token_count = pipe_tokenize(input_buf, token_arr);
        } else {
            token_count = tokenize_input(input_buf, token_arr);
            make_var(token_count, token_arr);
        }

        char *close_s[MAX_STR_LEN] = {"close-server"};
        if (ret == 0) {
            execute_command(close_s);
            deallocate();
            deallocate_background();
            break;
        }
        if (ret != -1) {
            if (token_count > 0 && (strlen(token_arr[0]) == 4 && strncmp("exit", token_arr[0], 4) == 0)) {
                //Call dellocate here.
                execute_command(close_s);
                deallocate();
                deallocate_background();
                break;
            }
        }


        /* Setting the size of the token count is a bit tricky,
        but in the case the we have a pipe, token_count is the
        number of commands, whilst with no pipe, token_count is
        the number of arguments.*/
        set_size(token_count);
        // Command execution
        if (token_count >= 1) {
            if (pipes == 1) {
                //The result of a fork.
                int result;
                //Number of arguments
                //The array that contains the pipes themselves.
                int pipe_fds[token_count][2];
                //The actual for loop.
                for (int proc = 0; proc < token_count; proc++) {
                    size_t argument_count = tokenize_input(token_arr[proc], temp_arr);
                    make_var(argument_count, temp_arr);
                    set_size(argument_count);
                    //Now you want to start piping and forking
                    if ((pipe(pipe_fds[proc])) == -1) {
                        display_error("", "pipe error");
                        exit(1);
                    }
                    result = fork();
                    if (result < 0) {
                        display_error("", "fork error");
                        exit(1);
                    } else if (result == 0) {
                        //You are in the child now
                        close(pipe_fds[proc][0]);
                        //Making sure that for the final process it goes to the actual
                        //output stream
                        if (proc == token_count - 1) {
                            dup2(original_stdout, STDOUT_FILENO);
                            close(pipe_fds[proc][1]);
                        } else {
                            dup2(pipe_fds[proc][1], STDOUT_FILENO);
                        }
                        for (int i = 0; i < proc; i++) {
                            close(pipe_fds[i][0]);
                        }
                        error = execute_command(temp_arr);
                        if (error == -1) {
                            display_error("ERROR: Builtin failed: ", temp_arr[0]);
                        }
                        if (proc < token_count) {
                            close(pipe_fds[proc][1]);
                        }
                        exit(0);
                    } 
                    close(pipe_fds[proc][1]);
                    dup2(pipe_fds[proc][0], STDIN_FILENO);
                    close(pipe_fds[proc][0]);
                }
                for (int i = 0; i < token_count; i++) {
                    wait(NULL);
                }
                dup2(original_stdin, STDIN_FILENO);
            } else {
                //This path is taken if we have no pipes as part of our commands.
                    if (token_count == 1 && strchr(token_arr[0], '=') != NULL) {
                        char temp[MAX_STR_LEN];
                        strncpy(temp, token_arr[0], strlen(token_arr[0]) + 1);
                        char *name = strtok(temp, "=");
                        char *value = strchr(token_arr[0], '=') + 1;
                        if (strlen(search(name)) == 0) {
                            insert(name, value);
                        } else {
                            change(name, value);
                        }
                    } else {
                        error = execute_command(token_arr);
                        if (error == -1) {
                            display_error("ERROR: Builtin failed: ", token_arr[0]);
                            //exit(1);
                        }
                    }
            }
        }
    }
    return 0;
}