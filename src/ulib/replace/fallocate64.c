// fallocate64.c

#include <ulib/base/base.h>

#include <errno.h>
#include <fcntl.h>

extern U_EXPORT int fallocate64(int fd, int mode, off_t offset, off_t len);

/* Reserve storage for the data of the file associated with FD. */

int fallocate64(int fd, int mode, off_t offset, off_t len) { return fallocate(fd, mode, offset, len); }
