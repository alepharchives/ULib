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

class U_EXPORT UEventTime : public UTimeVal {
public:

   struct timeval ctime;

   void reset() { ctime.tv_sec = ctime.tv_usec = 0L; }

   UEventTime(long sec = 0L, long usec = 0L) : UTimeVal(sec, usec)
      {
      U_TRACE_REGISTER_OBJECT(0, UEventTime, "%ld,%ld", sec, usec)

      reset();
      }

   virtual ~UEventTime()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UEventTime)
      }

   bool operator<(const UEventTime& t) const;

   // SERVICES

   bool isOld() const;
   void setCurrentTime();
   void setTimerVal(struct timeval* it_value);

   time_t expire() { return (ctime.tv_sec + tv_sec); }

   // -------------------------------------------
   // method VIRTUAL to define
   // -------------------------------------------
   // return value: -1 -> normal, 0 -> monitoring
   // -------------------------------------------

   virtual int handlerTime() { return -1; }

   friend U_EXPORT ostream& operator<<(ostream& os, const UEventTime& t);
   
#ifdef DEBUG
   const char* dump(bool reset) const;
#endif
};

#endif
