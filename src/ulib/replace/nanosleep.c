/* nanosleep.c */

#if !defined(U_ALL_C)
#  include <ulib/base/top.h>
#  ifdef HAVE_CONFIG_H
#     include <ulib/internal/config.h>
#  endif
#  include <ulib/base/bottom.h>
#  include <ulib/base/macro.h>
#endif

#include <errno.h>
#include <time.h>

/* Pause execution for a number of nanoseconds */

extern U_EXPORT int nanosleep(const struct timespec* requested_time, struct timespec* remaining);

U_EXPORT int nanosleep(const struct timespec* requested_time, struct timespec* remaining) { errno = ENOSYS; return -1; }
