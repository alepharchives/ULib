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

class U_EXPORT UEventFd {
public:

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   int fd, op_mask; // R_OK, W_OK -- NB: W_OK == 2, R_OK == 4...

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
