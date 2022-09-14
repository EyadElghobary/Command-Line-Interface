//Required Functions:
//1.Check Command similar to check builtin.
//2. Command for ls
//3. Command for cat
//4. Command for wc
//In the commands.h file you want to create an array that contains {ls, cat, wc}

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>

#include "commands.h"
#include "io_helpers.h"
#include "other_helpers.h"

int num_processes = 0;


/* For exiting the client*/
int exitStatus = 0;
void client_handler(int signal) {
    exitStatus = 1;
}


//This variable is used to help with the number outputted in [i] <num>
int proc_num = 0;

void add_proc(pid_t pid, char *cmd, char *full) {
    //This function returns the process number (not pid)
    process_arr[num_processes] = pid;
    proc_names[num_processes] = malloc(sizeof(char) * (MAX_STR_LEN + 1));
    strncpy(proc_names[num_processes], cmd, strlen(cmd) + 1);
    commands[num_processes] = malloc(sizeof(char) * (MAX_STR_LEN + 1));
    strncpy(commands[num_processes], full, strlen(full) + 1);
    num_processes += 1;
    proc_num += 1;
}

int get_proc_num() {
    return proc_num;
}

int get_proc() {
    return num_processes;
}

int search_proc(pid_t pid) {
    for (int i = 0; i < num_processes; i++) {
        if (process_arr[i] == pid) {
            return 1;
        }
    }
    return 0;
}

void check_all_process() {
    for (int i = 0; i < num_processes; i++) {
        int status;
        pid_t result = waitpid(process_arr[i], &status, WNOHANG);
        if (result > 0) {
            char buffer [100];
            snprintf(buffer, 100, "[%d]+  Done %s\n", i+1, commands[i]);
            //-10 is just a number that I chose to indicate
            //that the process is no longer running
            process_arr[i] = -10;
            proc_num -= 1;
            display_message(buffer);
        } else if (result == 0) {
            //display_message("still running\n");
        } else {
            // display_message("All process is done\n");
        }
    }
}

ssize_t kill_command(pid_t pid, int signum) {
    if (signum < 0 || signum > 31) {
        display_error("", "ERROR: Invalid signal specified");
        return -1;
    }
    if ((kill(pid, signum)) == -1) {
        display_error("", "ERROR: The process does not exist");
        return -1;
    }
    return 0;
}

void ps_command() {
    char s[MAX_STR_LEN];
    for (int i = 0; i < num_processes; i++) {
        if (process_arr[i] != -10) {
            display_message(proc_names[i]);
            display_message(" ");
            sprintf(s, "%d", process_arr[i]);
            display_message(s);
            display_message("\n");
            
        }
    }
}

void deallocate_background() {
    for (int i = 0; i < num_processes; i++) {
        free(proc_names[i]);
        free(commands[i]);
    }
}

int setup_client(char **tokens) {
    int client_sock = socket(AF_INET, SOCK_STREAM, 0);

    if (client_sock < 0) {
        display_error("", "socket error");
        return -1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(tokens[1]));
    if (inet_pton(AF_INET, tokens[2], &server_addr.sin_addr) < 1) {
        display_error("", "client: inet_pton");
        close(client_sock);
        return -1;
    }

    if (connect(client_sock, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1) {
        display_error("", "client: connect");
        close(client_sock);
        return -1;
    }
    return client_sock;
}

int send_command(char **tokens, int size) {
    int send_sock = setup_client(tokens);

    if (send_sock < 0) {
        display_error("", "socket error");
        return -1;
    }

    char msg[MAX_STR_LEN + 1];
    strncpy(msg, tokens[3], strlen(tokens[3]) + 1);
    for (int i = 4; i < size; i++) {
        strncat(msg, " ", 2);
        strncat(msg, tokens[i], strlen(tokens[i]));
    }
    int len = strlen(msg);
    msg[len] = '\n';
    msg[len + 1] = '\0';
    write(send_sock, msg, strlen(msg) + 1);
    return 0;
}

int start_client(char **tokens) {
    int client_sock = setup_client(tokens);
    
    signal(SIGINT, &client_handler);

    if (client_sock < 0) {
        display_error("", "socket error");
        return -1;
    }
    char line[MAX_STR_LEN + 1];
    
    while (fgets(line, MAX_STR_LEN + 1, stdin) != NULL) {
        if (exitStatus) break;
        int msg_len = strlen(line);
        write(client_sock, line, msg_len + 1);
    }
    return 0;
}


ssize_t startserver(char **tokens) {
    int size = get_size();

    if (size == 1) {
        display_error("", "ERROR: No port provided");
        return -1;
    }

    // Setting up our server socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0) {
        display_error("", "ERROR: socket error");
        return -1;
    }

    // Allows us to use the same port again
    int on = 1;
    int status = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR,
        (const char *) &on, sizeof(on));
    if (status < 0) {
        close(server_fd);
        display_error("", "ERROR: setsockopt");
        return -1;
    }

    // The address that we are binding too
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(tokens[1]));
    addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(addr.sin_zero), 0, 8);

    // Binding the server socket to the local machine's address
    if (bind(server_fd, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) == -1) {
        display_error("", "ERROR: binding error");
        return -1;
    }

    // Allows our server to listen to up MAX_CONNECTIONS connections.
    if (listen(server_fd, MAX_CONNECTIONS) == -1) {
        display_error("", "ERROR: listening error");
        return -1;
    }

    // We will create the peer address
    struct sockaddr_in client_addr;
    unsigned int client_len = sizeof(struct sockaddr_in);
    client_addr.sin_family = AF_INET;

    fd_set all_fds, listen_fds;
    FD_ZERO(&all_fds);
    FD_SET(server_fd, &all_fds);

    int max_fd = server_fd;

    // msg is a buffer that contains what we read from each client
    char msg[MAX_STR_LEN + 1];

    // These two variables are used for our clients
    int clients[MAX_CONNECTIONS];
    int client_size = 0;

    while(1) {
        listen_fds = all_fds;
        int nready = select(max_fd + 1, &listen_fds, NULL, NULL, NULL);
        if (nready == -1) {
            if (errno == EINTR) continue;
            display_error("", "ERROR: server select");
            break;
        }

        if (FD_ISSET(server_fd, &listen_fds)) {
            int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &client_len);
            if (client_fd < 0) {
                display_error("", "ERROR: accepting error");
                return -1;
            }

            if (client_fd > max_fd) {
                max_fd = client_fd;
            }

            clients[client_size] = client_fd;
            FD_SET(client_fd, &all_fds);
            client_size++;
        }
        
        for (int i = 0; i < client_size; i++) {
            if (!FD_ISSET(clients[i], &listen_fds)) {
                continue;
            }
            int num_read = read(clients[i], msg, MAX_STR_LEN);
            if (num_read <= 0) {
                clients[i] = -1;
            } else {
                display_message("\n");
                int len = strlen(msg);
                msg[len] = '\0';
                display_message(msg);
                display_message("mysh$ ");
                memset(msg, 0, MAX_STR_LEN);
            }
        }

        for (int i = 0; i < client_size; i++) {
            if (clients[i] == -1) {
                FD_CLR(clients[i], &all_fds);
                close(clients[i]);
            }
        }
    }
    close(server_fd);
    return 0;
}