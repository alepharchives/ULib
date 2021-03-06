// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    semaphore.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_SEMAPHORE_H
#define ULIB_SEMAPHORE_H 1

#include <ulib/internal/common.h>

#ifndef ENABLE_THREAD
#  define U_LOCKFILE
#  include <ulib/file.h>
#  ifdef __MINGW32__
typedef HANDLE sem_t;
#  else
typedef int sem_t;
#  endif
#endif

#ifdef __MINGW32__
typedef DWORD  timeout_t;
#  define MAX_SEM_VALUE 1000000
#else
typedef time_t timeout_t;
#  if !defined(U_LOCKFILE) && defined(HAVE_SEMAPHORE_H)
#     include <semaphore.h>
//    -------------------------------------------------------------
//    check for broken implementation on Linux (debian)...
//    -------------------------------------------------------------
#     if defined(LINUX) || defined(__LINUX__) || defined(__linux__)
#        include <linux/version.h>
#        ifndef KERNEL_VERSION
#           define KERNEL_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))
#        endif
#        ifndef LINUX_VERSION_CODE
#           error "You need to use at least 2.0 Linux kernel."
#        endif
#        if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,7)
#           define U_MAYBE_BROKEN_SEM_IMPL
#        endif
#     endif
//    -------------------------------------------------------------
/*
#  else
extern "C" {
int sem_wait(sem_t*__sem);
int sem_post(sem_t* __sem);
int sem_destroy(sem_t* __sem);
int sem_getvalue(sem_t* sem, int* sval);
int sem_init(sem_t* __sem, int __pshared, unsigned int __value):
}
*/
#  endif
#endif

#if !defined(U_LOCKFILE) && (!defined(HAVE_SEMAPHORE_H) || defined(U_MAYBE_BROKEN_SEM_IMPL)) // _LIBC // uClib
#  ifdef _POSIX_THREAD_PROCESS_SHARED
#     ifdef HAVE_PTHREAD_H
#        include <pthread.h>
#     else
//    -------------------------------------------------------------
extern "C" {
int pthread_mutex_lock(pthread_mutex_t* mutex);
int pthread_mutex_unlock(pthread_mutex_t* mutex);
int pthread_mutex_destroy(pthread_mutex_t* mutex);
int pthread_mutexattr_init(pthread_mutexattr_t* mutex_attr);
int pthread_mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t* mutex_attr); }
#     endif
#  endif
#endif

class UTimeVal;
class UServer_Base;

/**
 A semaphore is generally used as a synchronization object between multiple threads or to protect a limited and finite
 resource such as a memory or thread pool. The semaphore has a counter which only permits access by one or more threads
 when the value of the semaphore is non-zero. Each access reduces the current value of the semaphore by 1. One or more
 threads can wait on a semaphore until it is no longer 0, and hence the semaphore can be used as a simple thread
 synchronization object to enable one thread to pause others until the thread is ready or has provided data for them.
 Semaphores are typically used as a counter for protecting or limiting concurrent access to a given resource, such as
 to permitting at most "x" number of threads to use resource "y", for example
 */

class U_EXPORT USemaphore {
public:

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   USemaphore()
      {
      U_TRACE_REGISTER_OBJECT(0, USemaphore, "")

      next = 0;
      }

   /**
   The initial value of the semaphore can be specified. An initial value is often used when used to lock a finite
   resource or to specify the maximum number of thread instances that can access a specified resource.

   @param resource specify initial resource count or 1 default
   */

   void init(sem_t* ptr = 0, unsigned resource = 1);

   /**
   Destroying a semaphore also removes any system resources associated with it. If a semaphore has threads currently
   waiting on it, those threads will all continue when a semaphore is destroyed
   */

   ~USemaphore();

   /**
   Wait is used to keep a thread held until the semaphore counter is greater than 0. If the current thread is held, then
   another thread must increment the semaphore. Once the thread is accepted, the semaphore is automatically decremented,
   and the thread continues execution.

   @return false if timed out
   @param timeout period in milliseconds to wait
   */

   bool wait(timeout_t timeout);

   void lock() { (void) wait(0); }

   /**
   Posting to a semaphore increments its current value and releases the first thread waiting for the semaphore
   if it is currently at 0. Interestingly, there is no support to increment a semaphore by any value greater than 1
   to release multiple waiting threads in either pthread. Hence, if one wants to release a semaphore to enable multiple
   threads to execute, one must perform multiple post operations
   */

   void post();

   void unlock() { post(); }

#ifdef DEBUG
   const char* dump(bool) const;
#endif

protected:
   USemaphore* next;
#ifdef U_LOCKFILE
   UFile tmp;
#elif defined(__MINGW32__)
   HANDLE sem;
#else
   sem_t* sem;
#  ifdef U_MAYBE_BROKEN_SEM_IMPL
   pthread_mutex_t     mutex;
   pthread_mutexattr_t mutex_attr;
#  endif
   uint32_t map_size;
   bool broken;
#endif

   static USemaphore* first;
   
#if defined(DEBUG) || defined(U_TEST)
#  if defined(__MINGW32__) || defined(U_LOCKFILE) || defined(U_MAYBE_BROKEN_SEM_IMPL)
   int getValue() { return -1; }
#  else
   int getValue() { int value; sem_getvalue(sem, &value); return value; }
#  endif
#endif

   static bool checkForDeadLock(UTimeVal& time); // NB: check if process has restarted and it had a lock locked...
   
private:
   USemaphore(const USemaphore&)            {}
   USemaphore& operator=(const USemaphore&) { return *this; }

   friend class UServer_Base;
};

#endif
