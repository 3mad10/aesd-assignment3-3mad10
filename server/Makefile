OBJECTS = file_handling.o socket_handling.o aesdsocket.o
OUTPUT = aesdsocket

CC := $(CROSS_COMPILE)gcc

all: $(OBJECTS)
	$(CC) $^ -o aesdsocket

aesdsocket.o: aesdsocket.c file_handling.o socket_handling.o
	$(CC) -Wall -Wextra -g -c aesdsocket.c

file_handling.o: file_handling.c file_handling.h
	$(CC) -Wall -Wextra -g -c file_handling.c

socket_handling.o: socket_handling.c socket_handling.h server_cfg.h
	$(CC) -Wall -Wextra -g -c socket_handling.c

clean :
	rm -f $(OBJECTS) $(OUTPUT)