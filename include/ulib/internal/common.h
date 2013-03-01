// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    common.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_COMMON_H
#define ULIB_COMMON_H 1

// Manage file to include

#include <ulib/base/base.h>
#include <ulib/debug/macro.h>

#ifdef DEBUG
#  include <ulib/debug/trace.h>
#  include <ulib/debug/common.h>
#  include <ulib/debug/objectDB.h>
#  include <ulib/debug/error_memory.h>
#endif

#if !defined(HAVE_CONFIG_H) && !defined(U_LIBEXECDIR)
#  define U_LIBEXECDIR "/usr/libexec/ulib"
#endif

#if defined(__clang__) && defined(HAVE_PTHREAD_H) && defined(ENABLE_THREAD)
#include <pthread.h>

//typedef pthread_t        __gthread_t;
//typedef pthread_key_t    __gthread_key_t;
//typedef pthread_once_t   __gthread_once_t;
  typedef pthread_mutex_t  __gthread_mutex_t;
//typedef pthread_mutex_t  __gthread_recursive_mutex_t;
//typedef pthread_cond_t   __gthread_cond_t;
//typedef struct timespec  __gthread_time_t;
#endif

#include <cstdlib>
#include <ulib/internal/macro.h>
#include <ulib/internal/memory_pool.h>

// -------------------------------------------
// for gcc compiler strict aliasing behaviour
// -------------------------------------------
class UStringRep;

union uustringrep {
   ustringrep* p1;
   UStringRep* p2;
};
// -------------------------------------------

/* NB: in this way we don't capture the event 'DEAD OF SOURCE STRING WITH CHILD ALIVE'...
#define U_SUBSTR_INC_REF
*/

USING(std) // Common C++

// Init library

U_EXPORT void ULib_init();
#ifdef USE_LIBSSL
U_EXPORT void ULib_init_openssl();
#endif

#define U_ULIB_INIT(argv) U_SET_LOCATION_INFO, u_init_ulib(argv), ULib_init()

#endif
