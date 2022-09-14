CFLAGS = -g -Wall -Werror -fsanitize=address

all: mysh

mysh: mysh.o builtins.o commands.o io_helpers.o variables.o other_helpers.o
	gcc ${CFLAGS} -o $@ $^

%.o: %.c builtins.h commands.h io_helpers.h variables.h other_helpers.h
	gcc ${CFLAGS} -c $<

clean:
	rm *.o mysh
