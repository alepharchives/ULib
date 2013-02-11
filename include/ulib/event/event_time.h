// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    event_time.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_EVENT_TIME_H
#define ULIB_EVENT_TIME_H 1

#include <ulib/timeval.h>

#ifdef USE_LIBEVENT
#  include <ulib/libevent/event.h>
U_EXPORT struct event_base* u_ev_base;
#endif

class U_EXPORT UEventTime : public UTimeVal {
public:

   struct timeval ctime;

   void reset() { ctime.tv_sec = ctime.tv_usec = 0L; }

   UEventTime(long sec = 0L, long usec = 0L) : UTimeVal(sec, usec)
      {
      U_TRACE_REGISTER_OBJECT(0, UEventTime, "%ld,%ld", sec, usec)

      reset();

#  ifdef USE_LIBEVENT
      U_INTERNAL_ASSERT_POINTER(u_ev_base)

      pevent = U_NEW(UTimerEv<UEventTime>(*this));

      (void) UDispatcher::add(*pevent, *(UTimeVal*)this);
#  endif

      U_INTERNAL_DUMP("this = %p memory._this = %p", this, memory._this)
      }

   virtual ~UEventTime()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UEventTime)

#  ifdef USE_LIBEVENT
      UDispatcher::del(pevent);
                delete pevent;
#  endif

      U_INTERNAL_DUMP("this = %p memory._this = %p", this, memory._this)
      }

   bool operator<(const UEventTime& t) const __pure;

   // SERVICES

   void setCurrentTime();
   bool isOld() const __pure;
   bool isExpired() const __pure;
   void setTimerVal(struct timeval* it_value);

   time_t expire() { return (ctime.tv_sec + tv_sec); }

   // -------------------------------------------
   // method VIRTUAL to define
   // -------------------------------------------
   // return value: -1 -> normal, 0 -> monitoring
   // -------------------------------------------

   virtual int handlerTime() { return -1; }

#ifdef USE_LIBEVENT
   UTimerEv<UEventTime>* pevent;

   void operator()(int fd, short event);
#endif

   friend U_EXPORT ostream& operator<<(ostream& os, const UEventTime& t);

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

private:
   UEventTime(const UEventTime&) : UTimeVal() {}
   UEventTime& operator=(const UEventTime&)   { return *this; }
};

#endif
