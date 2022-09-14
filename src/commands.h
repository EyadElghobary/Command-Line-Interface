#ifndef __COMMANDS_H__
#define __COMMANDS_H__

#ifndef MAX_CONNECTIONS
    #define MAX_CONNECTIONS 12
#endif

#define MAX_STR_LEN 64

//Creates an ArrayList of the processes
pid_t process_arr[100];
char *proc_names[100];
char *commands[100];
//The size of the array
int num_processes;

struct client {
    int sock_fd;
    char buf[MAX_STR_LEN];
    int inbuf;
    // This variable is 0 if not a send and 1 if send.
    //int send;
    struct client *next;
};

/*Setters and Getters that help*/
// void set_servernum(int num);
// int get_servernum();
// void set_send(int value);
// int get_send();

// void sigint_handler(int code);

void client_handler(int signal);

void add_proc(pid_t pid, char *cmd, char *full);
int get_proc_num();
int get_proc();
int search_proc(pid_t);
void check_all_process();
ssize_t kill_command(pid_t pid, int signum);
void ps_command();
//This function is for deallocating our arrays.
void deallocate_background();

int setup_client(char **tokens);
int start_client(char **tokens);

int send_command(char **tokens, int size);

//int read_from_client(int sock_fd, char *buf, int *inbuf);
// int read_from_client(int sock_fd, char *buf); 
// int find_network_newline(const char *buf, int inbuf);
// int get_message(char **dst, char *src, int *inbuf);
// int accept_connection(int server_fd, struct client **clients, int *size);
ssize_t startserver(char **tokens);
#endif