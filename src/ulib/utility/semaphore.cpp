// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    semaphore.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/file.h>
#include <ulib/timeval.h>
#include <ulib/utility/semaphore.h>

USemaphore* USemaphore::first;

/**
The initial value of the semaphore can be specified. An initial value is often used when used to lock a finite
resource or to specify the maximum number of thread instances that can access a specified resource.

@param resource specify initial resource count or 1 default
*/

void USemaphore::init(sem_t* ptr, unsigned resource)
{
   U_TRACE(1, "USemaphore::init(%p,%u)", ptr, resource)

   U_CHECK_MEMORY

#ifdef U_LOCKFILE
   (void) tmp.mkTemp(0); // temporary  file for locking...
#elif defined(__MINGW32__)
   sem = ::CreateSemaphore((LPSECURITY_ATTRIBUTES)NULL, (LONG)resource, MAX_SEM_VALUE, (LPCTSTR)NULL);
#else
   if (ptr)
      {
      sem      = ptr;
      map_size = 0;
      }
   else
      {
      map_size = sizeof(sem_t);
      sem      = (sem_t*) UFile::mmap(&map_size);
      }

   /* initialize semaphore object sem to value, share it with other processes */
   broken = (U_SYSCALL(sem_init, "%p,%d,%u", sem, 1, resource) == -1); // 1 -> semaphore is shared between processes

#  ifdef U_MAYBE_BROKEN_SEM_IMPL
   if (broken)
      {
      (void) U_SYSCALL(pthread_mutexattr_init, "%p", &mutex_attr);
      (void) U_SYSCALL(pthread_mutexattr_setpshared, "%p,%p", &mutex_attr, PTHREAD_PROCESS_SHARED);
      (void) U_SYSCALL(pthread_mutex_init, "%p,%p", &mutex, &mutex_attr);

      return;
      }
#  endif
   if (ptr)
      {
      next  = first;
      first = this;

      U_INTERNAL_DUMP("first = %p next = %p", first, next)
      }
#endif
}

/**
Destroying a semaphore also removes any system resources associated with it. If a semaphore has threads currently
waiting on it, those threads will all continue when a semaphore is destroyed
*/

USemaphore::~USemaphore()
{
   U_TRACE_UNREGISTER_OBJECT(0, USemaphore)

#ifdef U_LOCKFILE
   (void) tmp.close();
#elif defined(__MINGW32__)
   ::CloseHandle(sem);
#else
#  ifdef U_MAYBE_BROKEN_SEM_IMPL
   if (broken) (void) U_SYSCALL(pthread_mutex_destroy, "%p", &mutex);
   else
#  endif
   /* Free resources associated with semaphore object sem */
   (void) U_SYSCALL(sem_destroy, "%p", sem);

   if (map_size) UFile::munmap(sem, map_size);
#endif
}

// NB: check if process has restarted and it had a lock locked...

bool USemaphore::checkForDeadLock(UTimeVal& time)
{
   U_TRACE(1, "USemaphore::checkForDeadLock(%p)", &time)

   bool sleeped = false;

#if !defined(__MINGW32__) && !defined(U_LOCKFILE)
   int value;
   sem_t* sem;

   for (USemaphore* item = first; item; item = item->next)
      {
      sem = item->sem;

      U_INTERNAL_ASSERT_POINTER(sem)

      if (U_SYSCALL(sem_getvalue, "%p,%p", sem, &value) == 0 && value > 0) continue;

      time.nanosleep();

      sleeped = true;

      if (U_SYSCALL(sem_getvalue, "%p,%p", sem, &value) == 0 && value > 0) continue;

      time.nanosleep();

      if (U_SYSCALL(sem_getvalue, "%p,%p", sem, &value) == 0 && value > 0) continue;

      (void) U_SYSCALL(sem_post, "%p", sem); /* Post sem */
      }
#endif

   U_RETURN(sleeped);
}

/**
Wait is used to keep a thread held until the semaphore counter is greater than 0. If the current thread is held, then
another thread must increment the semaphore. Once the thread is accepted, the semaphore is automatically decremented,
and the thread continues execution.

@return false if timed out
@param timeout period in milliseconds to wait
*/

bool USemaphore::wait(timeout_t timeout)
{
   U_TRACE(1, "USemaphore::wait(%lu)", timeout)

   U_CHECK_MEMORY

#ifdef U_LOCKFILE
   if (tmp.lock()) U_RETURN(true);
                   U_RETURN(false);
#elif defined(__MINGW32__)
   if (::WaitForSingleObject(sem, (timeout ? timeout : INFINITE)) == WAIT_OBJECT_0) U_RETURN(true);
                                                                                    U_RETURN(false);
#else
   int rc;

   U_INTERNAL_DUMP("value = %d", getValue())

   U_INTERNAL_ASSERT(getValue() <= 1)

#  ifdef U_MAYBE_BROKEN_SEM_IMPL
   if (broken) rc = U_SYSCALL(pthread_mutex_lock, "%p", &mutex);
   else
#  endif
   if (timeout == 0) rc = U_SYSCALL(sem_wait, "%p", sem);
   else
      {
      /* Wait for sem being posted */

      U_INTERNAL_ASSERT(u_now->tv_sec > 1260183779) // 07/12/2009

      struct timespec abs_timeout = { u_now->tv_sec + timeout / 1000, 0 };

      U_INTERNAL_DUMP("abs_timeout = { %d, %d }", abs_timeout.tv_sec, abs_timeout.tv_nsec)

      rc = U_SYSCALL(sem_timedwait, "%p,%p", sem, &abs_timeout);
      }

   U_RETURN(rc == 0);
#endif
}

/**
Posting to a semaphore increments its current value and releases the first thread waiting for the semaphore
if it is currently at 0. Interestingly, there is no support to increment a semaphore by any value greater than 1
to release multiple waiting threads in either pthread or the win32 API. Hence, if one wants to release
a semaphore to enable multiple threads to execute, one must perform multiple post operations
*/

void USemaphore::post()
{
   U_TRACE(1, "USemaphore::post()")

   U_CHECK_MEMORY

#ifdef U_LOCKFILE
   (void) tmp.unlock();
#elif defined(__MINGW32__)
   ::ReleaseSemaphore(sem, 1, (LPLONG)NULL);
#else
   U_INTERNAL_DUMP("value = %d", getValue())
   U_INTERNAL_ASSERT(getValue() <= 1)
#  ifdef U_MAYBE_BROKEN_SEM_IMPL
   if (broken) (void) U_SYSCALL(pthread_mutex_unlock, "%p", &mutex);
   else
#  endif
   (void) U_SYSCALL(sem_post, "%p", sem); /* Post sem */
#endif
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* USemaphore::dump(bool reset) const
{
#ifdef U_LOCKFILE
   *UObjectIO::os << "tmp (UFile " << (void*)&tmp   << ')';
#elif defined(__MINGW32__)
   *UObjectIO::os << "sem        " << (void*)sem;
#else
   *UObjectIO::os << "sem        " << (void*)sem    << '\n';
#  if defined(U_MAYBE_BROKEN_SEM_IMPL)
   *UObjectIO::os << "mutex      " << (void*)&mutex << '\n'
                  << "mutex_attr " << (void*)&mutex_attr;
#  endif
   *UObjectIO::os << "map_size   " << map_size      << '\n'
                  << "broken     " << broken;
#endif

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}
#endif
