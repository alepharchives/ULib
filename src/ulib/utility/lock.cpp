// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    lock.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/utility/lock.h>

void ULock::init(sem_t* ptr)
{
   U_TRACE(0, "ULock::init(%p)", ptr)

   U_CHECK_MEMORY

   sem = U_NEW(USemaphore);

   sem->init(ptr);
}

void ULock::destroy()
{
   U_TRACE(0, "ULock::destroy()")

   U_CHECK_MEMORY

   unlock();

   if (sem)
      {
      delete sem;
             sem = 0;
      }
}

void ULock::lock(timeout_t timeout)
{
   U_TRACE(0, "ULock::lock(%ld)", timeout)

   U_CHECK_MEMORY

   U_INTERNAL_DUMP("locked = %d", locked)

   if (++locked == 1)
      {
      if (sem &&
          sem->wait(timeout) == false)
          {
          locked = 0;
          }
      }
}

void ULock::unlock()
{
   U_TRACE(0, "ULock::unlock()")

   U_CHECK_MEMORY

   U_INTERNAL_DUMP("locked = %d", locked)

   if (--locked == 0)
      {
      if (sem) sem->unlock();
      }
}

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* ULock::dump(bool reset) const
{
   *UObjectIO::os << "locked          " << locked     << '\n'
                  << "sem (USemaphore " << (void*)sem << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
