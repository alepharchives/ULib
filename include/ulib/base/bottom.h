/* bottom.h */

#ifndef BOTTOM_H
#define BOTTOM_H 1

/* LFS */
#if !defined(_OFF_T_) && defined(__MINGW32__) && defined(_FILE_OFFSET_BITS) && _FILE_OFFSET_BITS == 64
#  define  _OFF_T_
typedef long long _off_t; /* Type of file sizes and offsets (LFS) */
typedef _off_t     off_t;
#endif

#ifdef _MSC_VER
/* Visual Studio hasn't inttypes.h so it doesn't know uint32_t */
typedef int int32_t;
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
typedef unsigned long long uint64_t;
typedef int mode_t;
#else /* _MSC_VER */
#  include <inttypes.h>
#endif /* _MSC_VER */

#if defined(SOLARIS) || defined(__hpux) || defined(MACOSX)
#  ifndef __EXTENSIONS__
#     define UNDEF__EXTENSIONS__
#     define __EXTENSIONS__ 1 /* sys/stat.h won't compile without this */
#  endif
#endif

#include <sys/stat.h>

#if defined(SOLARIS) || defined(__hpux) || defined(MACOSX)
#  ifdef UNDEF__EXTENSIONS__
#     undef __EXTENSIONS__
#  endif
#endif

#if defined(SOLARIS)
#  define _VA_LIST /* can't define it in stdio.h */
#endif

#include <stdio.h>

#if defined(SOLARIS)
#  undef _VA_LIST
#endif

#include <stdarg.h> /* this is the place to define _VA_LIST */

#ifndef ETC_PREFIX
#  ifdef _WIN32
#     define ETC_PREFIX "C:\\WINDOWS\\"
#  endif

#  ifndef ETC_PREFIX
#     define ETC_PREFIX "/etc/"
#  endif
#endif

#if __WORDSIZE >= 64 || defined(__arch64__)
#  define HAVE_ARCH64
#endif

#if defined(__CYGWIN__)
#  define sighandler_t _sig_func_ptr
#endif

#ifdef HAVE_STDINT_H
#  include <stdint.h>
#endif

#if defined(__MINGW32__)
#  define HAVE_WORKING_SOCKET_OPTION_SO_RCVTIMEO 1
#  define STRNCASECMP(x,y) strncasecmp((x),y,sizeof(y)-1)
#  include <ulib/base/win32/system.h>
#elif defined(__linux__)
#  define HAVE_WORKING_SOCKET_OPTION_SO_RCVTIMEO 1
#else
#  define _snprintf snprintf
#  define STRNCASECMP(x,y) strncasecmp(x,y,sizeof(y)-1)
#endif

#if defined(SOLARIS) || defined(__APPLE__)
#  if defined(MACOSX)
#     define HAVE_LFS 1
#  endif
#  define HAVE_STRTOULL  1
#  define HAVE_INET_NTOP 1
/*#define HAVE_NO_UNALIGNED_ACCESSES 1 :si con asm("ta 6")*/
extern char** environ;
typedef void (*__sighandler_t)(int);
typedef __sighandler_t sighandler_t;
#endif

#ifndef HAVE_NANOSLEEP
#  ifdef __cplusplus
extern "C" {
#  endif
int nanosleep(const struct timespec* req, struct timespec* rem);
#  ifdef __cplusplus
}
#  endif
#endif

#ifndef HAVE_SENDFILE64
#  ifdef __cplusplus
extern "C" {
#  endif
ssize_t sendfile(int out_fd, int in_fd, off_t* poffset, size_t count);
#  ifdef __cplusplus
}
#  endif
#endif

#ifndef HAVE_MREMAP
#  ifdef __cplusplus
extern "C" {
#  endif
void* mremap(void* old_address, size_t old_size , size_t new_size, int flags);
#  ifdef __cplusplus
}
#  endif
#endif

#ifndef HAVE_STRNDUP
#  ifdef __cplusplus
extern "C" {
#  endif
char* strndup(const char* s, size_t n);
#  ifdef __cplusplus
}
#  endif
#endif

#ifndef HAVE_STRPTIME
#  ifdef __cplusplus
extern "C" {
#  endif
char* strptime(const char* buf, const char* fmt, struct tm* tm);
#  ifdef __cplusplus
}
#  endif
#endif

#ifndef HAVE_MKDTEMP
#  ifdef __cplusplus
extern "C" {
#  endif
char* mkdtemp(char* template_name);
#  ifdef __cplusplus
}
#  endif
#endif

#ifndef HAVE_MEMRCHR
#  ifdef __cplusplus
extern "C" {
#  endif
void* memrchr(const void* s, int c, size_t count);
#  ifdef __cplusplus
}
#  endif
#endif

#if !defined(HAVE_GMTIME_R) && !defined(PTHREAD_H)
#  ifdef __cplusplus
extern "C" {
#  endif
struct tm* gmtime_r(const time_t* timep, struct tm* result);
#  ifdef __cplusplus
}
#  endif
#endif

#ifndef HAVE_TIMEGM
#  ifdef __cplusplus
extern "C" {
#  endif
time_t timegm(struct tm* tm);
#  ifdef __cplusplus
}
#  endif
#endif

#ifndef HAVE_DAEMON
#  ifdef __cplusplus
extern "C" {
#  endif
int daemon(int nochdir, int noclose);
#  ifdef __cplusplus
}
#  endif
#endif

#ifndef HAVE_PREAD
#  ifdef __cplusplus
extern "C" {
#  endif
ssize_t pread(int fd, void *buf, size_t count, off_t offset);
#  ifdef __cplusplus
}
#  endif
#endif

#ifndef HAVE_PREAD_PWRITE
#  ifdef __cplusplus
extern "C" {
#  endif
ssize_t pwrite(int fd, const void* buf, size_t count, off_t offset);
#  ifdef __cplusplus
}
#  endif
#endif

#ifndef HAVE_FNMATCH
#  ifdef __cplusplus
extern "C" {
#  endif
/* Bits set in the FLAGS argument to 'fnmatch'  */
#define FNM_PATHNAME    (1 << 0) /* No wildcard can ever match '/' */
#define FNM_NOESCAPE    (1 << 1) /* Backslashes don't quote special chars */
#define FNM_PERIOD      (1 << 2) /* Leading '.' is matched only explicitly */
#define FNM_LEADING_DIR (1 << 3) /* Ignore '/...' after a match */
#define FNM_CASEFOLD    (1 << 4) /* Compare without regard to case */

#define FNM_NOMATCH 1 /* Value returned by `fnmatch' if STRING does not match PATTERN */
#define FNM_NOSYS (-1)

int fnmatch(const char*, const char*, int);
#  ifdef __cplusplus
}
#  endif
#endif

#ifndef HAVE_FALLOCATE
#  ifdef __cplusplus
extern "C" {
#  endif
int fallocate(int fd, int mode, off_t offset, off_t len);
#  ifdef __cplusplus
}
#  endif
#endif

#ifdef HAVE_SSL
#  include <openssl/opensslv.h>
#  if   (OPENSSL_VERSION_NUMBER < 0x00905100L)
#     error "Must use OpenSSL 0.9.6 or later, Aborting..."
#  elif (OPENSSL_VERSION_NUMBER > 0x00908000L)
#     define HAVE_OPENSSL_98 1
#  elif (OPENSSL_VERSION_NUMBER > 0x00907000L)
#     define HAVE_OPENSSL_97 1
#  endif
#endif

#if !defined(PACKAGE) && defined(PACKAGE_NAME)
#  define PACKAGE PACKAGE_NAME
#endif
#if !defined(ULIB_VERSION) && defined(PACKAGE_VERSION)
#  define ULIB_VERSION PACKAGE_VERSION
#endif
#if !defined(REPORT_BUGS) && defined(PACKAGE_BUGREPORT)
#  define REPORT_BUGS PACKAGE_BUGREPORT
#endif

#endif
