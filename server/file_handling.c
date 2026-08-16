#include "file_handling.h"
#include <string.h>
#include <stdio.h>
void write_to_file(int fd, const char* buf, int len) {
    int ret;
    printf("Writing to fd = %d\n", fd);
    while((len != 0) && ((ret = write (fd, buf, len))!=0)) {
        if (ret == -1) {
            printf("error writing to file error %s\n",strerror(errno));
            if (errno == EINTR) continue;
            syslog(LOG_ERR, "Error Writing FIle");
            break;
        }
        len -= ret;
        buf += ret;
    }
}
