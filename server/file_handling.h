#ifndef FILE_HANDLING
#define FILE_HANDLING

#include <errno.h>
#include <syslog.h>

void write_to_file(int fd, const char* buf, int len);

#endif // FILE_HANDLING
