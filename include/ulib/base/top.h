/* from common C++ */

#ifndef TOP_H
#define TOP_H 1

#if defined(__CYGWIN__)
#  define _POSIX_REALTIME_SIGNALS
#  ifndef    _POSIX_THREADS
#     define _POSIX_THREADS
#  endif
#endif

#if defined(__APPLE__) && defined(__MACH__)
#  ifndef    MACOSX
#     define MACOSX
#     define _P1003_1B_VISIBLE
#  endif
#  ifndef    _PTHREADS
#     define _PTHREADS 1
#  endif
#endif

#if defined(__FreeBSD__)
#  ifndef    __BSD_VISIBLE
#     define __BSD_VISIBLE 1
#  endif
#endif

#ifdef _AIX
#  ifndef    _ALL_SOURCE
#     define _ALL_SOURCE
#  endif
#endif

#ifdef  __hpux
#  ifndef    _XOPEN_SOURCE_EXTENDED
#     define _XOPEN_SOURCE_EXTENDED
#  endif
#  ifndef    _INCLUDE_LONGLONG
#     define _INCLUDE_LONGLONG
#  endif
#endif

#if defined(__sun) || defined(__SUN__)
#  define SOLARIS
#endif

/*
#ifndef   _REENTRANT
#  define _REENTRANT 1
#endif

#ifndef   _THREAD_SAFE
#  define _THREAD_SAFE 1
#endif

#if !defined(_XOPEN_SOURCE) && !defined(__FreeBSD__) && !defined(__OpenBSD__) && !defined(__MACH__) && !defined(__NetBSD__) 
#    define  _XOPEN_SOURCE 600
#endif
*/

#ifdef __GNUC__
#  if  __GNUC__ > 1 && !defined(__STRICT_ANSI__) && !defined(__PEDANTIC__)
#     define  DYNAMIC_LOCAL_ARRAYS
#     ifndef __cplusplus
#        include <stdbool.h> /* C99 only */
#     endif
#  endif
#  ifndef    _GNU_SOURCE
#     define _GNU_SOURCE 1 /* bring GNU as close to C99 as possible */
#  endif
#elif !defined(FLEX_SCANNER) && !defined(__FreeBSD__) && !defined(MACOSX) /* _POSIX_SOURCE too restrictive */
/*
   Header to request specific standard support. Before including it, one
   of the following symbols must be defined (1003.1-1988 isn't supported):

     SUV_POSIX1990  for 1003.1-1990
     SUV_POSIX1993  for 1003.1b-1993 - real-time
     SUV_POSIX1996  for 1003.1-1996
     SUV_SUS1       for Single UNIX Specification, v. 1 (UNIX 95)
     SUV_SUS2       for Single UNIX Specification, v. 2 (UNIX 98)
     SUV_SUS3       for Single UNIX Specification, v. 3
*/
#  undef  SUV_POSIX1990
#  undef  SUV_POSIX1993
#  undef  SUV_POSIX1996
#  undef  SUV_SUS1
#  define SUV_SUS2 1
#  undef  SUV_SUS3
#  include "suvreq.h"
#endif

#endif
