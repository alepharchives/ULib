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

#include <ulib/string.h>

#include <ulib/event/event_fd.h>
#include <ulib/event/event_time.h>

/* NB: to force use of select()
#ifdef HAVE_EPOLL_WAIT
#undef HAVE_EPOLL_WAIT
#endif
*/

#ifndef __MINGW32__
#  include <sys/select.h>
#  ifdef HAVE_EPOLL_WAIT
#     include <sys/epoll.h>
#     define MAX_EVENTS 32
#  endif
#endif

class UServer_Base;
class UClientImage_Base;

// interface to select() and/or epoll()

class U_EXPORT UNotifier {
public:

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   // COSTRUTTORI

   UNotifier()
      {
      U_TRACE_REGISTER_OBJECT(0, UNotifier, "")

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

   static void init();
   static void clear();
   static void insert(UEventFd* handler_event);
   static void erase( UEventFd* handler_event, bool flag_reuse);

   static bool isHandler(UEventFd* handler_event)
      {
      U_TRACE(0, "UNotifier::isHandler(%p)", handler_event)

      for (UNotifier* item = first; item; item = item->next)
         {
         if (item->handler_event_fd == handler_event) U_RETURN(true);
         }

      U_RETURN(false);
      }

   // return false if there are no more events registered

   static bool waitForEvent(UEventTime* timeout);

   // READ - WRITE

   // param timeoutMS specified the timeout value, in milliseconds.
   // A negative value indicates wait for event with no timeout, i.e. an infinite wait

   static int waitForRead( int fd, int timeoutMS = -1);
   static int waitForWrite(int fd, int timeoutMS = -1);

   static uint32_t  read(int fd,       char* buf, int count = U_SINGLE_READ, int timeoutMS = -1);
   static uint32_t write(int fd, const char* str, int count,                 int timeoutMS = -1);

   // Call function for all entry

   static void callForAllEntry(bPFpv function);

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
   static int result, fd_set_max;
   static int fd_read_cnt, fd_write_cnt;
   static fd_set fd_set_read, fd_set_write;
   static bool exit_loop_wait_event_for_signal;

   static void preallocate(int n);
   static void waitForEvent(int fd_max, fd_set* read_set, fd_set* write_set, UEventTime* timeout);

#ifdef USE_POLL
   static void waitForEvent(struct pollfd* ufds, int timeoutMS = -1);
#endif

#if defined(HAVE_EPOLL_WAIT) && !defined(HAVE_LIBEVENT)
   static int epollfd;
   static struct epoll_event events[MAX_EVENTS];
#else
   // nfds is the highest-numbered file descriptor in any of the three sets, plus 1.

   static int  getNFDS();
   static void setNFDS(int fd);

   // rimuove i descrittori di file diventati invalidi (possibile con EPIPE)

   static void removeBadFd();
#endif

private:
   void outputEntry(ostream& os) const U_NO_EXPORT;

   static void eraseHandler(UEventFd* handler_event) U_NO_EXPORT;
   static void eraseItem(UNotifier* item, bool flag_reuse) U_NO_EXPORT;

#ifndef HAVE_LIBEVENT
   static bool handlerResult(int& n, UNotifier*  i,
                                     UNotifier** ptr,
                                     UEventFd* handler_event,
                                     bool bread, bool bwrite, bool bexcept) U_NO_EXPORT; 
#endif

   UNotifier(const UNotifier&)            {}
   UNotifier& operator=(const UNotifier&) { return *this; }

   friend class UServer_Base;
   friend class UClientImage_Base;
};

#endif
