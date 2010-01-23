// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    timer.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/timer.h>
#include <ulib/utility/interrupt.h>

bool             UTimer::async;
UTimer*          UTimer::pool;
UTimer*          UTimer::first;
struct itimerval UTimer::timerval;

UTimer::~UTimer()
{
   U_TRACE_UNREGISTER_OBJECT(0, UTimer)

   U_INTERNAL_DUMP("next = %p, alarm = %p", next, alarm)

   if (next)  delete next;
   if (alarm) delete alarm;
}

void UTimer::init(bool _async)
{
   U_TRACE(0, "UTimer::init(%b)", _async)

   if ((async = _async)) UInterrupt::insert(             SIGALRM, (sighandler_t)UTimer::handlerAlarm); // async signal
   else                  UInterrupt::setHandlerForSignal(SIGALRM, (sighandler_t)UTimer::handlerAlarm);
}

void UTimer::stop()
{
   U_TRACE(1, "UTimer::stop()")

   timerval.it_value.tv_sec  = 0;
   timerval.it_value.tv_usec = 0;

   (void) U_SYSCALL(setitimer, "%d,%p,%p", ITIMER_REAL, &timerval, 0);
}

U_NO_EXPORT void UTimer::insertEntry()
{
   U_TRACE(1, "UTimer::insertEntry()")

   U_CHECK_MEMORY

   alarm->setCurrentTime();

   UTimer** ptr = &first;

   while (*ptr)
      {
      if (*this < **ptr) break;

      ptr = &(*ptr)->next;
      }

   next = *ptr;
   *ptr = this;

   U_INTERNAL_DUMP("timer = %O", U_OBJECT_TO_TRACE(*first))

   U_ASSERT(invariant())
}

inline void UTimer::callHandlerTime()
{
   U_TRACE(0, "UTimer::callHandlerTime()")

   U_INTERNAL_DUMP("u_now = %#4D (next alarm expire) = %#4D", u_now.tv_sec, (next ? next->alarm->expire() : 0))

   int result = alarm->handlerTime(); // chiama il gestore dell'evento scadenza temporale

   U_INTERNAL_DUMP("result = %d", result)

   // return value:
   // ---------------
   // -1 - normal
   //  0 - monitoring
   // ---------------

   if (result == 0) insertEntry(); // monitoraggio: si aggiunge il nodo alla lista con la scadenza aggiornata al nuovo tempo assoluto...
   else
      {
      // per rientranza gestione memoria si evita new e/o delete nella gestione del segnale SIGALRM

      if (async)
         {
         delete alarm;
                alarm = 0;
         }

      // lo si mette all'inizio della lista degli item da riutilizzare...

      next = pool;
      pool = this;

      U_INTERNAL_DUMP("pool = %O", U_OBJECT_TO_TRACE(*pool))
      }
}

void UTimer::setTimer()
{
   U_TRACE(1, "UTimer::setTimer()")

   UTimer* item;
   UTimer** ptr = &first;

   (void) U_SYSCALL(gettimeofday, "%p,%p", &u_now, 0);

   U_INTERNAL_DUMP("u_now = { %ld %6ld }", u_now.tv_sec, u_now.tv_usec)

   while ((item = *ptr))
      {
      if (item->alarm->isOld())
         {
         *ptr = item->next; // toglie il nodo scaduto dalla lista...

         item->callHandlerTime();

         ptr = &first;

         continue;
         }

      item->alarm->setTimerVal(&timerval.it_value);

      goto set_itimer;
      }

   timerval.it_value.tv_sec = timerval.it_value.tv_usec = 0L;

set_itimer:

   U_INTERNAL_DUMP("timerval.it_value = { %ld %6ld }", timerval.it_value.tv_sec, timerval.it_value.tv_usec)

   (void) U_SYSCALL(setitimer, "%d,%p,%p", ITIMER_REAL, &timerval, 0);
}

void UTimer::insert(UEventTime* a, bool set_timer)
{
   U_TRACE(0, "UTimer::insert(%O,%b)", U_OBJECT_TO_TRACE(*(UTimeVal*)a), set_timer)

   // NB: non si puo' riutilizzare uno stesso oggetto gia' inserito nel timer...

   U_INTERNAL_ASSERT_EQUALS(a->ctime.tv_sec,0)
   U_INTERNAL_ASSERT_EQUALS(a->ctime.tv_usec,0)

   // settare un allarme a piu' di due mesi e' molto sospetto...

   U_INTERNAL_ASSERT_MINOR(a->tv_sec,(2 * 30 * 24 * 3600))

   UTimer* item;

   if (pool)
      {
      // per rientranza gestione memoria si evita new e/o delete nella gestione del segnale SIGALRM

      if (pool->alarm) delete pool->alarm;

      item = pool;
      pool = pool->next;
      }
   else
      {
      item = U_NEW(UTimer);
      }

   item->alarm = a;

   // NB: si mette il nodo in ordine nella lista con la scadenza settata al tempo assoluto...

   item->insertEntry();

   if (set_timer) setTimer();
}

void UTimer::erase(UEventTime* a, bool flag_reuse, bool set_timer)
{
   U_TRACE(0, "UTimer::erase(%O,%b,%b)", U_OBJECT_TO_TRACE(*a), flag_reuse, set_timer)

   U_INTERNAL_ASSERT_POINTER(first)

   UTimer* item =  first;
   UTimer** ptr = &first;

   do {
      if (item->alarm == a)
         {
         U_INTERNAL_DUMP("alarm = %O", U_OBJECT_TO_TRACE(*item->alarm))

         *ptr = item->next; // lo si toglie dalla lista scadenze...

         U_ASSERT(invariant())

         // e lo si mette nella lista degli item da riutilizzare...

         if (flag_reuse)
            {
            item->next = pool;
            pool       = item;

            delete item->alarm;
                   item->alarm = 0;

            U_INTERNAL_DUMP("pool = %O", U_OBJECT_TO_TRACE(*pool))
            }
         else
            {
            item->next  = 0;
            item->alarm = 0;

            delete item;
            }

         break;
         }

      ptr = &(*ptr)->next;
      }
   while ((item = *ptr));

   if (set_timer) setTimer();
}

void UTimer::clear(bool clean_alarm)
{
   U_TRACE(0, "UTimer::clear(%b)", clean_alarm)

   U_INTERNAL_DUMP("first = %p pool = %p", first, pool)

   UTimer* item = first;

   if (first)
      {
      if (clean_alarm) do { item->alarm = 0; } while ((item = item->next));

      delete first;
             first = 0;
      }

   if (pool)
      {
      item = pool;

      if (clean_alarm) do { item->alarm = 0; } while ((item = item->next));

      delete pool;
             pool = 0;
      }
}

// STREAM

U_EXPORT ostream& operator<<(ostream& os, const UTimer& t)
{
   U_TRACE(0+256, "UTimer::operator<<(%p,%p)", &os, &t)

   os.put('(');
   os.put(' ');

   if (t.alarm) os << *t.alarm;
   else         os << (void*)&t;

   for (UTimer* item = t.next; item; item = item->next)
      {
      os.put(' ');

      if (item->alarm) os << *item->alarm;
      else             os << (void*)item;
      }

   os.put(' ');
   os.put(')');

   return os;
}

#ifdef DEBUG

bool UTimer::invariant()
{
   U_TRACE(0, "UTimer::invariant()")

   if (first)
      {
      for (UTimer* item = first; item->next; item = item->next)
         {
#     if defined(__unix__) && !defined(__CYGWIN__)
         U_INTERNAL_ASSERT_EQUALS((*item < *item->next),true)
#     endif
         }
      }

   U_RETURN(true);
}

void UTimer::printInfo(ostream& os)
{
   U_TRACE(0+256, "UTimer::printInfo(%p)", &os)

   os << "first = ";

   if (first) os << *first;
   else       os << (void*)first;

   os << "\npool  = ";

   if (pool) os << *pool;
   else      os << (void*)pool;

   os << "\n";
}

#  include <ulib/internal/objectIO.h>

const char* UTimer::dump(bool reset) const
{
   *UObjectIO::os << "timerval                 " << "{ { "  << timerval.it_interval.tv_sec
                                                 << " "     << timerval.it_interval.tv_usec
                                                 << " } { " << timerval.it_value.tv_sec
                                                 << " "     << timerval.it_value.tv_usec
                                                                 << " } }\n"
                  << "pool         (UTimer     " << (void*)pool  << ")\n"
                  << "first        (UTimer     " << (void*)first << ")\n"
                  << "next         (UTimer     " << (void*)next  << ")\n"
                  << "alarm        (UEventTime " << (void*)alarm << ")";

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
