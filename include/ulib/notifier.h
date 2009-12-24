// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    notifier.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_NOTIFIER_H
#define ULIB_NOTIFIER_H

#include <ulib/event/event_time.h>
#include <ulib/event/event_fd.h>

#include <iostream>

#ifndef __MINGW32__
#  include <sys/select.h>
#endif

#ifdef HAVE_WORKING_SOCKET_OPTION_SO_RCVTIMEO
#  define U_INTERNAL_TIMEOUT        -1 // manage delegate timeout to setsockopt(SO_RCVTIMEO)...
#else
#  define U_INTERNAL_TIMEOUT U_TIMEOUT
#endif

// #ifndef __FDS_BITS
// #  define __FDS_BITS(fd_set) ((fd_set)->fds_bits)
// #endif

class UString;
class UServer_Base;
class UClientImage_Base;

// interface to select()

class U_EXPORT UNotifier {
public:

   static fd_set fd_set_read, fd_set_write;
   static int result, fd_set_max, fd_read_cnt, fd_write_cnt;

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   // COSTRUTTORI

   UNotifier()
      {
      U_TRACE_REGISTER_OBJECT(0, UNotifier, "", 0)

      next             = 0;
      handler_event_fd = 0;
      }

   ~UNotifier();

   // SERVICES

   static bool empty()
      {
      U_TRACE(0, "UNotifier::empty()")

      bool esito = (first == 0);

      U_RETURN(esito);
      }

   static void clear();

   static void     erase(UEventFd* handler_event, bool flag_reuse);
   static void    insert(UEventFd* handler_event);
   static bool isHandler(UEventFd* handler_event);

   // rimuove i descrittori di file diventati invalidi (possibile con EPIPE)

   static void removeBadFd();

   // return false if there are no more events registered

   static bool waitForEvent(UEventTime* timeout);

#ifdef USE_POLL
   static void waitForEvent(struct pollfd* ufds, int timeoutMS = -1);
#endif

   // READ - WRITE

   // param timeoutMS specified the timeout value, in milliseconds.
   // A negative value indicates wait for event with no timeout, i.e. an infinite wait

   static int waitForRead( int fd, int timeoutMS = -1);
   static int waitForWrite(int fd, int timeoutMS = -1);

   static uint32_t  read(int fd,       char* buf, int count = U_SINGLE_READ, int timeoutMS = U_INTERNAL_TIMEOUT);
   static uint32_t write(int fd, const char* str, int count,                 int timeoutMS = -1);

   // STREAM

   friend U_EXPORT ostream& operator<<(ostream& os, const UNotifier& n);

#ifdef DEBUG
   static void printInfo(ostream& os);
   const char* dump(bool reset) const;
#endif

protected:
   UNotifier* next;
   UEventFd* handler_event_fd;

   static UNotifier* pool;
   static UNotifier* first;
   static bool exit_loop_wait_event_for_signal;

   static void waitForEvent(int fd_max, fd_set* read_set, fd_set* write_set, UEventTime* timeout);

private:
          void outputEntry(ostream& os) const U_NO_EXPORT;
   static bool handlerResult(int& n, UNotifier* i, UNotifier** ptr, UEventFd* handler_event, bool bread, bool bwrite, bool flag_handler_call) U_NO_EXPORT; 

   friend class      UServer_Base;
   friend class UClientImage_Base;

   UNotifier(const UNotifier&)            {}
   UNotifier& operator=(const UNotifier&) { return *this; }
};

#endif
