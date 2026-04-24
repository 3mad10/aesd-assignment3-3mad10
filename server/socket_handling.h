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

#include "server_cfg.h"

int setup_socket(int port, int sock_type);

#endif