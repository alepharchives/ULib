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
#include <ulib/container/gen_hash_map.h>

/* NB: to force use of select()
#ifdef HAVE_EPOLL_WAIT
#undef HAVE_EPOLL_WAIT
#endif
*/

#ifndef __MINGW32__
#  include <sys/select.h>
#  ifdef HAVE_EPOLL_WAIT
#     include <sys/epoll.h>
#  endif
#endif

/*
#if defined(U_NO_SSL) && defined(HAVE_EPOLL_WAIT) && !defined(HAVE_LIBEVENT)
#  define U_SCALABILITY
#endif
*/

#include <ulib/event/event_fd.h>
#include <ulib/event/event_time.h>

// Functor used by UGenericHashMap class to generate a hashcode for an object of type <int>
template <> struct UHashCodeFunctor<int> { uint32_t operator()(const int& fd) const { return (fd + ((fd >> 2) | (fd << 30))); } };

class UThread;
class USocket;
class UTCPSocket;
class UServer_Base;
class UClientImage_Base;

// interface to select() and epoll()

class U_EXPORT UNotifier {
public:

#ifdef HAVE_EPOLL_WAIT
   static struct epoll_event* pevents;
#endif
   static int min_connection, num_connection, max_connection;

   // SERVICES

   static void init();
   static void clear(bool bthread);
   static void erase( UEventFd* handler_event);
   static void modify(UEventFd* handler_event);
   static void insert(UEventFd* handler_event);
   static void callForAllEntryDynamic(bPFpv function);

   static bool empty()
      {
      U_TRACE(0, "UNotifier::empty()")

      U_INTERNAL_ASSERT_POINTER(lo_map_fd)
      U_INTERNAL_ASSERT_POINTER(hi_map_fd)

      if (num_connection == 0)
         {
         U_ASSERT(hi_map_fd->empty())

         U_RETURN(true);
         }

      U_RETURN(false);
      }

   static bool isDynamicConnection(int fd)
      {
      U_TRACE(0, "UNotifier::isDynamicConnection(%d)", fd)

      if (num_connection > min_connection && find(fd)) U_RETURN(true);

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

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   static int nfd_ready; // the number of file descriptors ready for the requested I/O
   static void* pthread;
   static UEventFd** lo_map_fd;
   static UGenericHashMap<int,UEventFd*>* hi_map_fd; // maps a fd to a node pointer

#ifdef HAVE_LIBEVENT
#elif defined(HAVE_EPOLL_WAIT)
   static int epollfd;
   static struct epoll_event*  events;
#else
   static UEventFd* first;
   static fd_set fd_set_read, fd_set_write;
   static int fd_set_max, fd_read_cnt, fd_write_cnt;

   static int  getNFDS();     // nfds is the highest-numbered file descriptor in any of the three sets, plus 1.
   static void removeBadFd(); // rimuove i descrittori di file diventati invalidi (possibile con EPIPE)
#endif

#ifdef USE_POLL
   static struct pollfd fds[1];
   static int waitForEvent(int timeoutMS = -1);
#endif

   static UEventFd* find(int fd);
   static int       waitForEvent(int fd_max, fd_set* read_set, fd_set* write_set, UEventTime* timeout);

private:
   static void handlerDelete(UEventFd* item) U_NO_EXPORT;

#ifndef HAVE_LIBEVENT
   static void handlerResult(UEventFd* handler_event, bool bread, bool bexcept) U_NO_EXPORT; 
#endif

   UNotifier(const UNotifier&)            {}
   UNotifier& operator=(const UNotifier&) { return *this; }

   friend class USocket;
   friend class UTCPSocket;
   friend class UServer_Base;
   friend class UClientImage_Base;
};

#endif
