// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    lock.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_LOCK_H
#define ULIB_LOCK_H 1

#include <ulib/utility/semaphore.h>

class U_EXPORT ULock {
public:

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   // Costruttori

   ULock()
      {
      U_TRACE_REGISTER_OBJECT(0, ULock, "", 0)

      sem    = 0;
      locked = false;
      }

   ~ULock()
      {
      U_TRACE_UNREGISTER_OBJECT(0, ULock)

      destroy();
      }

   // SERVICES

   void destroy();
   void init(sem_t* ptr);

   void   lock();
   void unlock();

   bool isLocked()
      {
      U_TRACE(0, "ULock::isLocked()")

      U_RETURN(locked);
      }

   bool isShared()
      {
      U_TRACE(0, "ULock::isShared()")

      bool result = (sem != 0); 

      U_RETURN(result);
      }

   // STREAM

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   USemaphore* sem;
   bool locked; // manage lock recursivity...

private:
   ULock(const ULock&)            {}
   ULock& operator=(const ULock&) { return *this; }
};

#endif
