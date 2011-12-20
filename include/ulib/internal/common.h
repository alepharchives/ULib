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

#  define U_ULIB_INIT(argv)   U_SET_LOCATION_INFO, u_init_ulib(argv), u_debug_init(), ULib_init()
#else
#  define U_ULIB_INIT(argv)   U_SET_LOCATION_INFO, u_init_ulib(argv),                 ULib_init()
#endif

#if !defined(HAVE_CONFIG_H) && !defined(U_LIBEXECDIR)
#  define U_LIBEXECDIR "/usr/libexec/ulib"
#endif

#if defined(__clang__) && defined(HAVE_PTHREAD_H)
#include <pthread.h>

//typedef pthread_t        __gthread_t;
//typedef pthread_key_t    __gthread_key_t;
//typedef pthread_once_t   __gthread_once_t;
  typedef pthread_mutex_t  __gthread_mutex_t;
//typedef pthread_mutex_t  __gthread_recursive_mutex_t;
//typedef pthread_cond_t   __gthread_cond_t;
//typedef struct timespec  __gthread_time_t;
#endif

// Manage memory pool

#include <cstdlib>
#include <ulib/internal/memory_pool.h>

#define U_MEMORY_ALLOCATOR \
   void* operator new(  size_t sz)  { U_INTERNAL_ASSERT(sz <= U_MAX_SIZE_PREALLOCATE); return UMemoryPool::pop(U_SIZE_TO_STACK_INDEX(sz)); } \
   void* operator new[](size_t sz)  { return UMemoryPool::_malloc(sz); }
#define U_MEMORY_DEALLOCATOR \
   void  operator delete(  void* _ptr, size_t sz) { U_INTERNAL_ASSERT(sz <= U_MAX_SIZE_PREALLOCATE); UMemoryPool::push(_ptr, U_SIZE_TO_STACK_INDEX(sz)); } \
   void  operator delete[](void* _ptr, size_t sz) { UMemoryPool::_free(_ptr, sz); }

#define U_MALLOC_GEN(  sz)                UMemoryPool::_malloc(sz)
#define U_FREE_GEN(ptr,sz)                UMemoryPool::_free(ptr,sz)

#define U_MALLOC(  sz)                    UMemoryPool::pop(      U_SIZE_TO_STACK_INDEX(sz))
#define U_FREE(ptr,sz)                    UMemoryPool::push(ptr, U_SIZE_TO_STACK_INDEX(sz))

#define U_MALLOC_TYPE(  type)     (type*) UMemoryPool::pop(      U_SIZE_TO_STACK_INDEX(sizeof(type)))
#define U_FREE_TYPE(ptr,type)             UMemoryPool::push(ptr, U_SIZE_TO_STACK_INDEX(sizeof(type)))

#define U_MALLOC_STR(sz,capacity)         UMemoryPool::_malloc_str(sz, capacity)
#define U_FREE_STR(ptr, sz)               UMemoryPool::_free_str(ptr, sz)

#define U_MALLOC_N(  n,type)      (type*) UMemoryPool::_malloc((n) * sizeof(type))
#define U_CALLOC_N(  n,type)      (type*) memset(UMemoryPool::_malloc((n) * sizeof(type)), 0, (n) * sizeof(type))
#define U_FREE_N(ptr,n,type)              UMemoryPool::_free(ptr, (n) * sizeof(type))

#define U_MALLOC_VECTOR(  n,type) (type**) UMemoryPool::_malloc(       (n) * sizeof(void*))
#define U_CALLOC_VECTOR(  n,type) (type**) memset(UMemoryPool::_malloc((n) * sizeof(void*)), 0, (n) * sizeof(void*))
#define U_FREE_VECTOR(ptr,n,type)          UMemoryPool::_free(ptr,     (n) * sizeof(void*))

/* in this way we don't capture the event 'dead of source string with child alive'...
#define U_SUBSTR_INC_REF
*/

// for gcc compiler strict aliasing problem...

class UStringRep;

union uustringrep {
   ustringrep* p1;
   UStringRep* p2;
};

#include <ulib/internal/macro.h>

USING(std) // Common C++

// Init library

U_EXPORT void ULib_init();
#ifdef HAVE_SSL
U_EXPORT void ULib_init_openssl();
#endif

#endif
