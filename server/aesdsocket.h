#ifndef AESDSOCKET_H
#define AESDSOCKET_H

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <syslog.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>

#include "file_handling.h"
#include "socket_handling.h"

int send_received_data(int cfd);
void send_chunk(int cfd, const char* buf, int len);
int handle_conn(int cfd);
void save_received_data(const char* recv_buff, int n);

#endif // AESDSOCKET_H
