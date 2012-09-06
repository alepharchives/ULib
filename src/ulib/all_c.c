/* all.c */

#define U_ALL_C

/*
#define DEBUG_DEBUG
#include <ulib/base/base.h>
*/

#ifdef __MINGW32__
#  include "base/win32/mingw32.c"
/*
#  include "base/win32/strtoull.c"
*/
#else
#  include <ulib/base/base.h>
#  ifndef HAVE_DAEMON
#     include "replace/daemon.c"
#  endif
#endif

#ifndef HAVE_GETOPT_LONG
#  include "replace/getopt_long.c"
#endif

/*
#undef  DEBUG_DEBUG
#define U_INTERNAL_TRACE(format,args...)
#define U_INTERNAL_PRINT(format,args...)
*/

#include "base/base.c"
#include "base/hash.c"
#include "base/utility.c"
#include "base/base_error.c"
#include "base/lzo/minilzo.c"
#include "base/coder/cbase64.c"
#include "base/coder/cescape.c"
#include "base/coder/chexdump.c"
#include "base/coder/curl_coder.c"
#include "base/coder/cxml_coder.c"
#include "base/coder/cquoted_printable.c"
/*
#include "base/rsort.c"
#include "base/mkqsort.c"
*/

#ifndef HAVE_STRNDUP
#  include "replace/strndup.c"
#endif

#ifndef HAVE_STRPTIME
#  include "replace/strptime.c"
#endif

#ifndef HAVE_NANOSLEEP
#  include "replace/nanosleep.c"
#endif

#ifndef HAVE_MREMAP
#  include "replace/mremap.c"
#endif

#ifndef HAVE_SENDFILE64
#  include "replace/sendfile.c"
#endif

#ifndef HAVE_MKDTEMP
#  include "replace/mkdtemp.c"
#endif

#ifndef HAVE_MEMRCHR
#  include "replace/memrchr.c"
#endif

#ifndef HAVE_GMTIME_R
#  include "replace/gmtime.c"
#endif

#ifndef HAVE_TIMEGM
#  include "replace/timegm.c"
#endif

#ifndef HAVE_FALLOCATE
#  include "replace/fallocate.c"
#endif

#ifndef HAVE_FALLOCATE64
#  include "replace/fallocate64.c"
#endif

#ifndef HAVE_PREAD
#  include "replace/pread.c"
#endif

#ifndef HAVE_ASSERT_H
#  include "replace/assert.c"
#endif

#ifndef USE_SEMAPHORE
#  include "replace/sem.c"
#endif

#ifdef DEBUG
#  include "base/base_trace.c"
#endif

#ifdef USE_LIBSSL
#  include "base/ssl/dgst.c"
#  include "base/ssl/cdes3.c"
#endif

#ifdef ENABLE_ZIP
/*
#undef  U_INTERNAL_TRACE
#define U_INTERNAL_TRACE(format,args...) \
{ char u_internal_buf[8192]; (void) sprintf(u_internal_buf, format"\n", args); \
(void) write(STDERR_FILENO, u_internal_buf, strlen(u_internal_buf)); }
#undef  U_INTERNAL_PRINT
#define U_INTERNAL_PRINT(format,args...) U_INTERNAL_TRACE(format,args)
*/
#  include "base/zip/dostime.c"
#  include "base/zip/inflate.c"
#  include "base/zip/pushback.c"
#  include "base/zip/ziptool.c"
#endif

#ifdef USE_LIBZ
#  include "base/coder/cgzio.c"
#endif
