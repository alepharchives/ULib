// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    event_fd.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_EVENT_FD_H
#define ULIB_EVENT_FD_H 1

#ifdef USE_LIBEVENT
#  include <ulib/libevent/event.h>
#endif

// -------------------------------------------------------------------------------------------------------------------------
// EPOLLET is edge-triggered (alas SIGIO, when that descriptor transitions from not ready to ready, the kernel notifies you)
// -------------------------------------------------------------------------------------------------------------------------
#ifndef EPOLLET
#define EPOLLET    0
#endif
#ifndef EPOLLIN
#define EPOLLIN    0x001
#endif
#ifndef EPOLLOUT
#define EPOLLOUT   0x004
#endif
#ifndef EPOLLRDHUP
#define EPOLLRDHUP 0x2000
#endif

#define U_READ_IN    (uint32_t)EPOLLIN
#define U_WRITE_OUT  (uint32_t)(EPOLLOUT | EPOLLET)

#define U_NOTIFIER_OK      0
#define U_NOTIFIER_DELETE -1

class U_EXPORT UEventFd {
public:

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   int fd;
   uint32_t op_mask; // [ U_READ_IN | U_WRITE_OUT ]

   UEventFd()
      {
      U_TRACE_REGISTER_OBJECT(0, UEventFd, "")
   
      fd      = 0;
      op_mask = U_READ_IN;

#  ifdef USE_LIBEVENT
      pevent = 0;
#  endif
      }

   virtual ~UEventFd()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UEventFd)

#  ifdef USE_LIBEVENT
      if (pevent)
         {
         UDispatcher::del(pevent);
                   delete pevent;
         }
#  endif
      }

   // method VIRTUAL to define

   virtual int handlerRead()           { return U_NOTIFIER_DELETE; }
   virtual int handlerWrite()          { return U_NOTIFIER_DELETE; }
   virtual int handlerError(int state) { return U_NOTIFIER_DELETE; }

   virtual void handlerDelete()        { delete this; }

#ifdef USE_LIBEVENT
   UEvent<UEventFd>* pevent;

   void operator()(int fd, short event);
#endif

#ifdef DEBUG
   const char* dump(bool reset) const { return "..."; } 
#endif

private:
   UEventFd(const UEventFd&)            {}
   UEventFd& operator=(const UEventFd&) { return *this; }
};

#endif
