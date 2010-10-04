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

#  define U_ULIB_INIT(argv)   U_SET_LOCATION_INFO, u_init(argv), u_debug_init(), ULib_init()
#else
#  define U_ULIB_INIT(argv)   U_SET_LOCATION_INFO, u_init(argv),                 ULib_init()
#endif

// Manage memory pool

#include <cstdlib>

#ifdef U_MEMORY_POOL
#  include <ulib/internal/memory_pool.h>

#  define U_MEMORY_ALLOCATOR \
   void* operator new(  size_t sz)  { U_INTERNAL_ASSERT(sz <= U_MAX_SIZE_PREALLOCATE); return UMemoryPool::pop(U_SIZE_TO_STACK_INDEX(sz)); } \
   void* operator new[](size_t sz)  { return UMemoryPool::_malloc(sz); }
#  define U_MEMORY_DEALLOCATOR \
   void  operator delete(  void* _ptr, size_t sz) { U_INTERNAL_ASSERT(sz <= U_MAX_SIZE_PREALLOCATE); UMemoryPool::push(_ptr, U_SIZE_TO_STACK_INDEX(sz)); } \
   void  operator delete[](void* _ptr, size_t sz) { UMemoryPool::_free(_ptr, sz); }

#  define U_MALLOC(  sz)                     UMemoryPool::pop(      U_SIZE_TO_STACK_INDEX(sz))
#  define U_FREE(ptr,sz)                     UMemoryPool::push(ptr, U_SIZE_TO_STACK_INDEX(sz))

#  define U_MALLOC_TYPE(  type)     (type*)  UMemoryPool::pop(      U_SIZEOF_TO_STACK_INDEX(type))
#  define U_FREE_TYPE(ptr,type)              UMemoryPool::push(ptr, U_SIZEOF_TO_STACK_INDEX(type))

#  define U_MALLOC_STR(sz,capacity)          UMemoryPool::_malloc_str(sz, capacity)
#  define U_FREE_STR(ptr, sz)                UMemoryPool::_free_str(ptr, sz)

#  define U_MALLOC_N(  n,type)      (type*)  UMemoryPool::_malloc(   (n) * sizeof(type))
#  define U_FREE_N(ptr,n,type)               UMemoryPool::_free(ptr, (n) * sizeof(type))

#  define U_MALLOC_VECTOR(  n,type) (type**) UMemoryPool::_malloc(   (n) * sizeof(type*))
#  define U_FREE_VECTOR(ptr,n,type)          UMemoryPool::_free(ptr, (n) * sizeof(type*))
#else
#  define U_MAX_SIZE_PREALLOCATE 4096

#  define U_MEMORY_ALLOCATOR
#  define U_MEMORY_DEALLOCATOR

#  define U_MALLOC(  sz)                U_SYSCALL(malloc, "%u", sz)
#  define U_FREE(ptr,sz)                U_SYSCALL_VOID(free, "%p", ptr)

#  define U_MALLOC_TYPE(  type) (type*) U_SYSCALL(malloc, "%u", sizeof(type))
#  define U_FREE_TYPE(ptr,type)         U_SYSCALL_VOID(free, "%p", ptr)

#  define U_MALLOC_STR(size,capacity)   U_SYSCALL(malloc, "%u", (capacity = size))
#  define U_FREE_STR(ptr, sz)           U_SYSCALL_VOID(free, "%p", ptr)

#  define U_MALLOC_N(  n,type)          new type[n]
#  define U_FREE_N(ptr,n,type)          delete[] ptr

#  define U_MALLOC_VECTOR(  n,type)     new type*[n]
#  define U_FREE_VECTOR(ptr,n,type)     delete[] ptr
#endif

// string representation (for gcc compiler strict aliasing problem...)

/* in this way we don't capture the event 'dead of source string with child alive'...
#define U_SUBSTR_INC_REF
*/

typedef struct ustringrep {
#ifdef DEBUG
   ustringrep* _this;
#endif
#if defined(U_SUBSTR_INC_REF) || defined(DEBUG)
   ustringrep* parent; // manage substring for increment reference of source string
#  ifdef DEBUG
   int32_t child;      // manage substring for capture event 'DEAD OF SOURCE STRING WITH CHILD ALIVE'...
#  endif
#endif
   uint32_t _length, _capacity;
   int32_t references;
   const char* str;
} ustringrep;

class UStringRep;

union uustringrep {
   ustringrep* p1;
   UStringRep* p2;
};

#include <ulib/internal/macro.h>

USING(std) // Common C++

extern U_EXPORT void ULib_init(); // Init library

#endif
