#include "file_handling.h"

void write_to_file(int fd, const char* buf, int len) {
    int ret;
    while((len != 0) && ((ret = write (fd, buf, len))!=0)) {
        if (ret == -1) {
            if (errno == EINTR) continue;
            syslog(LOG_ERR, "Error Writing FIle");
            break;
        }
        len -= ret;
        buf += ret;
    }
}
