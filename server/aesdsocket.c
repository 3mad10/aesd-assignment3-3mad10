

#include "aesdsocket.h"
#include "server_cfg.h"


bool g_server_running = true;

static void sigint_handler (int signo)
{
    if (signo == SIGINT)
        DEBUG_LOG("Caught signal handler for SIGINT");
    else if (signo == SIGTERM)
        DEBUG_LOG("Caught signal handler for SIGTERM");
    else {
        /* this should never happen */
        exit (EXIT_FAILURE);
    }
    syslog(LOG_INFO ,"Caught signal, exiting");
    g_server_running = false;
}

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main() {
    int sfd, cfd;
    struct sockaddr_storage peer_addr;
    socklen_t peer_addr_size;
    char s[INET6_ADDRSTRLEN];
    int err;
    struct sigaction termination_action;

    
    memset(&termination_action, 0, sizeof(termination_action));
    termination_action.sa_handler=sigint_handler;

    err = sigaction(SIGTERM, &termination_action, NULL);
    if (err != 0) {
        DEBUG_LOG("Failed to set signal handler for SIGTERM");
        exit (EXIT_FAILURE);
    }

    err = sigaction(SIGINT, &termination_action, NULL);
    if (err != 0) {
        DEBUG_LOG("Failed to set signal handler for SIGINT");
        exit (EXIT_FAILURE);
    }


    sfd = setup_socket(SERVER_PORT, SOCK_STREAM);
    if (sfd == -1) {
        DEBUG_LOG("Failed to Setup Socket with error %s", strerror(errno));
        syslog(LOG_ERR ,"Failed to Setup Socket with error %s", strerror(errno));
        exit (EXIT_FAILURE);
    }

    err = listen(sfd, BACKLOG); 
    if (err == -1) {
        close(sfd);
        DEBUG_LOG("Listen failed with error %s", strerror(errno));
        syslog(LOG_ERR ,"Listen failed with error %s", strerror(errno));
        exit (EXIT_FAILURE);
    }

    printf("[Server] waiting for connections...\n");

    while(g_server_running) {
        peer_addr_size = sizeof(peer_addr);
        cfd = accept(sfd, (struct sockaddr *)&peer_addr, &peer_addr_size);
        if (cfd == -1) {
            DEBUG_LOG("Accept failed with error %s", strerror(errno));
            syslog(LOG_ERR ,"Accept failed with error %s", strerror(errno));
            break;
        }

        inet_ntop(peer_addr.ss_family,
            get_in_addr((struct sockaddr *)&peer_addr),
            s, sizeof s);
        printf("[Server] got connection from %s\n", s);
        syslog(LOG_INFO ,"Accepted connection from %s", s);

        
        err = handle_conn(cfd);
        if (err == -1) {
            DEBUG_LOG("Connection failed with error %s", strerror(errno));
            syslog(LOG_ERR ,"Connection failed with error %s", strerror(errno));
        }

        close(cfd);

        printf("[Server] Closed connection from %s\n", s);
        syslog(LOG_INFO ,"Closed connection from %s", s);
    }
    err = remove(RECEIVED_SOCKET_DATA_PATH);
    if (err != 0) {
        DEBUG_LOG("Failed to remove file %s", RECEIVED_SOCKET_DATA_PATH);
        syslog(LOG_ERR ,"Failed to remove file %s", RECEIVED_SOCKET_DATA_PATH);
    }
    err = shutdown(sfd, SHUT_RDWR);
    if (err != 0) {
        DEBUG_LOG("Failed to shutdown socket sfd = %d with error : %s", sfd, strerror(errno));
        syslog(LOG_ERR ,"Failed to shutdown socket sfd = %d with error : %s", sfd, strerror(errno));
    }
    err = shutdown(cfd, SHUT_RDWR);
    if (err != 0) {
        DEBUG_LOG("Failed to shutdown socket cfd = %d with error : %s", cfd, strerror(errno));
        syslog(LOG_ERR ,"Failed to shutdown socket cfd = %d with error : %s", cfd, strerror(errno));
    }
    exit (EXIT_SUCCESS);
    remove(RECEIVED_SOCKET_DATA_PATH);

    return 0;
}

int send_received_data(int cfd) {
    int n;
    int fd;
    char transmit_buff[TRN_BUFF_SIZE];
    DEBUG_LOG("Sending data to client");
    fd = open(RECEIVED_SOCKET_DATA_PATH, O_RDONLY);
    while ((n = read (fd, &transmit_buff[0], sizeof(transmit_buff))) > 0) {
        DEBUG_LOG("n = %d" , n);
        DEBUG_LOG("char[0] = %c , char[n-2] = %c" , transmit_buff[0], transmit_buff[n-2]);
        if (n == -1) {
            syslog(LOG_ERR, "Error Sending Chunk to client");
            DEBUG_LOG("send failed with error", strerror(errno));
        }
        send_chunk(cfd, transmit_buff, n);
    }
    close(fd);
    return 0;
}

void send_chunk(int cfd, const char* buf, int len) {
    int n;
    DEBUG_LOG("Send chunk with params fd = %d len = %d" ,cfd , len);
    DEBUG_LOG("First char = %c" ,buf[0]);
    do
    {
        n = send(cfd, buf, len, 0);
        if (n == -1) {
            syslog(LOG_ERR, "Error Sending Chunk to client");
            DEBUG_LOG("send failed with error", strerror(errno));
            break;
        }
        DEBUG_LOG("n = %d", n);
        len = len - n;
    } while(len > 0);

}

int handle_conn(int cfd) {
    int n;
    char* recv_buff = (char *)malloc(RCV_CHUNK_SIZE);
    long unsigned int total_size = 0;
    long unsigned int capacity = RCV_CHUNK_SIZE;
    DEBUG_LOG("allocated buffer with size %d", capacity);
    if (recv_buff == NULL) return -1;

    while(((n = recv(cfd, &recv_buff[total_size], capacity-total_size, 0))!=0)) {
        DEBUG_LOG("received %d bytes", n);
        if (n == -1) {
            if (errno == EINTR) continue;
            DEBUG_LOG("Error receiving data");
            break;
        }
        total_size += n;
        if (memchr(&recv_buff[total_size - n], '\n', n) != NULL) {
            DEBUG_LOG("saving recv_buff byte[0] = %c to byte[%ld] = %c", recv_buff[0], total_size, recv_buff[total_size - 2]);
            save_received_data((char*)&recv_buff[0], total_size);
            send_received_data(cfd);
            total_size = 0;
        }
        if (total_size > capacity - REALLOC_LIMIT) {
            DEBUG_LOG("Reallocating buffer");
            recv_buff = (char *)realloc(recv_buff,
                capacity + RCV_CHUNK_SIZE);
            capacity += RCV_CHUNK_SIZE;
            if (recv_buff == NULL) {
                DEBUG_LOG("Error reallocating receive buffer");
                return -1;
            }
            DEBUG_LOG("New buffer size = %d byte", capacity);
        }
    }
    free(recv_buff);
    
    if(n == -1) {
        return -1;
    }

    return 0;
}

void save_received_data(const char* recv_buff, int n) {
    int fd;

    fd = open(RECEIVED_SOCKET_DATA_PATH, O_CREAT | O_WRONLY | O_APPEND, 0664);
    if (fd==-1) {
        syslog(LOG_ERR, "Could not open file %s", RECEIVED_SOCKET_DATA_PATH);
        DEBUG_LOG("Could not open file %s", RECEIVED_SOCKET_DATA_PATH);
    }
    DEBUG_LOG("Created file with fd = %d", fd);
    write_to_file(fd, &recv_buff[0], n);
    close(fd);
}

