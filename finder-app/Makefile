
CC := $(CROSS_COMPILE)gcc

writer: writer.c
	$(CC) -Wall -Wextra -g writer.c -o writer

writer.o : writer.c
	cc -c writer.c

clean :
	rm -f writer