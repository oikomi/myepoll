CC=gcc
CFLAGS=-O2 -W -Werror -Wall
PROG=myepoll
OBJS=myepoll.o


all: $(PROG)


%.o: %.c
	        $(CC) -c $(CFLAGS) $< -o $@


$(PROG): $(OBJS)
	        $(CC) $(OBJS) -o $(PROG) -lpthread


.PHONY:clean


clean:
	        rm -rf *.o $(PROG)
