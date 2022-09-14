#ifndef __OTHER_HELPERS_H__
#define __OTHER_HELPERS_H__


int size;

void set_size(size_t token_count);
int get_size();
int cd_helper(char *str);
int ls_helper(char *path, char *f_arg, int d_arg);
int pipe_count(char **tokens);


#endif