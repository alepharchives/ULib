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

#define U_NOTIFIER_OK      0
#define U_NOTIFIER_DELETE -1
#define U_NOTIFIER_UPDATE -2

#define U_READ_IN    0x001 // NB: same as EPOLLIN
#define U_WRITE_OUT  0x004 // NB: same as EPOLLOUT

class U_EXPORT UEventFd {
public:

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   int fd, op_mask; // U_READ_IN | U_WRITE_OUT

            UEventFd() {}
   virtual ~UEventFd() {}

   // method VIRTUAL to define

   virtual int handlerRead()  { return U_NOTIFIER_DELETE; }
   virtual int handlerWrite() { return U_NOTIFIER_DELETE; }

private:
   UEventFd(const UEventFd&)            {}
   UEventFd& operator=(const UEventFd&) { return *this; }
};

#endif
