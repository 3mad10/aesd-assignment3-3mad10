#ifndef socket_handling_H
#define socket_handling_H

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <syslog.h>
#include <errno.h>
#include <fcntl.h>
#include "stdbool.h"
#include <string.h>
#include <sys/ioctl.h>
#include "server_cfg.h"
#include "file_handling.h"
#include "types.h"
#include "aesd_ioctl.h"

int setup_socket(int port, int sock_type);
int wait_for_connection(int sfd, char* client_addr, int addr_len);
int echo_conn(struct job_data* data);

#endif