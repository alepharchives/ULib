// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    event_signal.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_EVENT_SIGNAL_H
#define ULIB_EVENT_SIGNAL_H 1

class U_EXPORT UEventSignal {
public:

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

            UEventSignal() {}
   virtual ~UEventSignal() {}

   // method VIRTUAL to define

   virtual int handlerSignal() { return -1; }

private:
   UEventSignal(const UEventSignal&)            {}
   UEventSignal& operator=(const UEventSignal&) { return *this; }
};

#endif
