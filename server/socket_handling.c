#include "socket_handling.h"

int setup_socket(int port, int sock_type) {
    int err;
    int sfd;
    struct addrinfo hints, *res;
    char port_str[20];
    int opts = 1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;  // use IPv4 or IPv6, whichever
    hints.ai_socktype = sock_type;
    hints.ai_flags = AI_PASSIVE; 

    sprintf(port_str, "%d", port);

    err = getaddrinfo(NULL, port_str, &hints, &res);
    if (err != 0) {
        DEBUG_LOG("Failed to get address with errno %s", gai_strerror(err));
        exit (EXIT_FAILURE);
    }

    sfd = socket(res->ai_family, res->ai_socktype,res->ai_protocol);
    if (sfd == -1){
        DEBUG_LOG("Socket creation failed with error %s", strerror(errno));
        exit (EXIT_FAILURE);
    }

	err = setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opts, sizeof(int));
	if (err == -1) 
	{
		DEBUG_LOG("Error in setsockopt");
        syslog(LOG_ERR ,"Setting Socket Options failed with error %s", strerror(errno));

	}

    err = bind(sfd, res->ai_addr, res->ai_addrlen);
    if (err != 0) {
        close(sfd);
        DEBUG_LOG("Failed to bind: %s", gai_strerror(err));
        freeaddrinfo(res);
        exit (EXIT_FAILURE);
    }

    freeaddrinfo(res);

    return sfd;
}
