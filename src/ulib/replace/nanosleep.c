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

U_EXPORT int nanosleep(const struct timespec* requested_time, struct timespec* remaining)
{
   /* Some code calls select() with all three sets empty, nfds zero,
    * and a non-NULL timeout as a fairly portable way to sleep with subsecond precision.
    */

   ((struct timespec*)requested_time)->tv_nsec /= 1000; /* nanoseconds to microseconds */

   int result = select(0, 0, 0, 0, (struct timeval*)requested_time);

   ((struct timespec*)requested_time)->tv_nsec *= 1000; /* microseconds to nanoseconds */

   *remaining = *requested_time;

   return result;
}
