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

#include <ulib/container/vector.h>

/* NB: to force use of select()
#ifdef HAVE_EPOLL_WAIT
#undef HAVE_EPOLL_WAIT
#endif
*/
/* NB: to force not use of pthread
#ifdef HAVE_PTHREAD_H
#undef HAVE_PTHREAD_H
#endif
*/

#ifndef __MINGW32__
#  include <sys/select.h>
#  ifdef HAVE_EPOLL_WAIT
#     include <sys/epoll.h>
#     define MAX_EVENTS 512
#  endif
#endif

#include <ulib/event/event_fd.h>
#include <ulib/event/event_time.h>

class UThread;
class USocket;
class UTCPSocket;
class UServer_Base;
class UClientImage_Base;

// interface to select() and epoll()

class U_EXPORT UNotifier : public UVector<UEventFd*> {
public:

   // COSTRUTTORI

   UNotifier(uint32_t n) : UVector<UEventFd*>(n)
      {
      U_TRACE_REGISTER_OBJECT(0, UNotifier, "")

      U_INTERNAL_ASSERT_EQUALS(pthis,0)
      }

   ~UNotifier()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UNotifier)
      }

   // SERVICES

   static void clear();
   static void init(uint32_t n = 64U);
   static void erase( UEventFd* handler_event);
   static void modify(UEventFd* handler_event);
   static void insert(UEventFd* handler_event, bool bstatic = true);

   static bool empty()
      {
      U_TRACE(0, "UNotifier::empty()")

      U_INTERNAL_ASSERT_POINTER(pthis)

      if (first || pthis->_length) U_RETURN(false);

      U_RETURN(true);
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

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   static UEventFd* first;
   static UNotifier* pthis;

#ifdef HAVE_PTHREAD_H
   static UThread* pthread;
#endif

#ifdef USE_POLL
   static struct pollfd fds[1];
   static int waitForEvent(int timeoutMS = -1);
#endif

   static int waitForEvent(int fd_max, fd_set* read_set, fd_set* write_set, UEventTime* timeout);

#ifdef HAVE_LIBEVENT
#elif defined(HAVE_EPOLL_WAIT)
   static int epollfd;
   static struct epoll_event events[MAX_EVENTS];

   static bool find(int fd) __pure;
#else
   static int fd_set_max;
   static int fd_read_cnt, fd_write_cnt;
   static fd_set fd_set_read, fd_set_write;

   static int  getNFDS();     // nfds is the highest-numbered file descriptor in any of the three sets, plus 1.
   static void removeBadFd(); // rimuove i descrittori di file diventati invalidi (possibile con EPIPE)
#endif

   static void callForAllEntryDynamic(bPFpv function);

private:
   static void eraseItem(    UEventFd** ptr, UEventFd* item) U_NO_EXPORT;
   static void handlerDelete(UEventFd** ptr, UEventFd* item) U_NO_EXPORT;

#ifndef HAVE_LIBEVENT
   static bool handlerResult(int& n, UEventFd** ptr, UEventFd* handler_event, bool bread, bool bwrite, bool bexcept) U_NO_EXPORT; 
#endif

   UNotifier(const UNotifier&) : UVector<UEventFd*>() {}
   UNotifier& operator=(const UNotifier&)             { return *this; }

   friend class USocket;
   friend class UTCPSocket;
   friend class UServer_Base;
   friend class UClientImage_Base;
};

#endif
