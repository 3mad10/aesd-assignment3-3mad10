#ifndef SERVER_CFG_H
#define SERVER_CFG_H

#define DEBUG_LOG(msg,...) printf("[DEBUG] " msg "\n" , ##__VA_ARGS__)
#define RCV_CHUNK_SIZE 1024
#define TRN_BUFF_SIZE 1024
#define SERVER_PORT 9000  // the port users will be connecting to
#define BACKLOG 10
#define RECEIVED_SOCKET_DATA_PATH "/var/tmp/aesdsocketdata"
#define REALLOC_LIMIT 2

#endif // SERVER_CFG
