// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    timer.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_TIMER_H
#define ULIB_TIMER_H

#include <ulib/event/event_time.h>

// Il notificatore degli eventi usa questa classe per notificare una scadenza temporale rilevata da select()

class U_EXPORT UTimer {
public:
   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator

   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   // COSTRUTTORI

   UTimer()
      {
      U_TRACE_REGISTER_OBJECT(0, UTimer, "")

      next  = 0;
      alarm = 0;
      }

   ~UTimer();

   // SERVICES

   static bool empty()
      {
      U_TRACE(0, "UTimer::empty()")

      bool result = (first == 0);

      U_RETURN(result);
      }

   static bool isRunning()
      {
      U_TRACE(0, "UTimer::isRunning()")

      bool result = (timerval.it_value.tv_sec  != 0 ||
                     timerval.it_value.tv_usec != 0);

      U_RETURN(result);
      }

   static void stop();
   static void setTimer();
   static void init(bool async = true);
   static void clear(bool clean_alarm);

   static void insert(UEventTime* alarm,                  bool set_timer = true);
   static void  erase(UEventTime* alarm, bool flag_reuse, bool set_timer = true);

   static bool isHandler(UEventTime* _alarm)
      {
      U_TRACE(0, "UTimer::isHandler(%p)", _alarm)

      for (UTimer* item = first; item; item = item->next)
         {
         PREFETCH_ATTRIBUTE(item->next, 0)

         if (item->alarm == _alarm) U_RETURN(true);
         }

      U_RETURN(false);
      }

   // manage signal

   static RETSIGTYPE handlerAlarm(int signo)
      {
      U_TRACE(0, "[SIGALRM] UTimer::handlerAlarm(%d)", signo)

      setTimer();
      }

   // STREAM

   friend U_EXPORT ostream& operator<<(ostream& os, const UTimer& t);

#if defined(DEBUG) || defined(U_TEST)
   static bool invariant();
#endif
#ifdef DEBUG
   static void printInfo(ostream& os);
   const char* dump(bool reset) const;
#endif

protected:
   UTimer* next;
   UEventTime* alarm;

   static bool async;
   static UTimer* pool;  // lista scadenze da cancellare o riutilizzare
   static UTimer* first; // lista scadenze
   static struct itimerval timerval;

private:
          void insertEntry() U_NO_EXPORT;
          void outputEntry(ostream& os) const U_NO_EXPORT;
   inline void callHandlerTime() U_NO_EXPORT;

   bool operator<(const UTimer& t) const { return (*alarm < *t.alarm); }

   UTimer(const UTimer&)            {}
   UTimer& operator=(const UTimer&) { return *this; }
};

#endif
