#include <syslog.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

int main(int argc, char *argv[]) {
    ssize_t ret, nr;
    size_t len;
    const char* buf= argv[2];
    if(3 != argc){
        syslog(LOG_ERR, "The input must be 2 arguments in format: </path/to/file> <data>");
        return -1;
    }

    int fd = open(argv[1], O_CREAT | O_WRONLY | O_TRUNC | O_SYNC, 0664);
    if (-1 == fd) {
        syslog(LOG_ERR, "Could not open file %s", argv[0]);
        return -1;
    }
    len = strlen(argv[2]);
    while((len != 0) && ((ret = write (fd, buf, len))!=0)) {
        if (ret == -1) {
            if (errno == EINTR) continue;
            syslog(LOG_ERR, "Error Writing FIle");
            break;
        }
        len -= ret;
        buf += ret;
        syslog(LOG_DEBUG, "Writing %s to %s", argv[2], argv[1]);

    }
    close(fd);
    return 0;
}