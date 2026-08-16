OBJECTS = file_handling.o socket_handling.o aesdsocket.o
DRIVER_DIR := ../aesd-char-driver

CC ?= $(CROSS_COMPILE)gcc
CFLAGS ?= -Wall -Wextra -g
INCLUDES ?= -I$(DRIVER_DIR)
TARGET ?= aesdsocket
LDFLAGS ?=

all: $(OBJECTS)
	$(CC) $(LDFLAGS) $^ -o $(TARGET)

aesdsocket.o: aesdsocket.c file_handling.o socket_handling.o
	$(CC) $(CFLAGS) $(INCLUDES) -c aesdsocket.c

file_handling.o: file_handling.c file_handling.h
	$(CC) $(CFLAGS) $(INCLUDES) -c file_handling.c

socket_handling.o: socket_handling.c socket_handling.h server_cfg.h
	$(CC) $(CFLAGS) $(INCLUDES) -c socket_handling.c

clean :
	rm -f $(OBJECTS) $(TARGET)