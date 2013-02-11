// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    timeval.h
//
// = AUTHOR
//    Stefanot Casazza
//
// ============================================================================

#ifndef ULIB_TIMEVAL_H
#define ULIB_TIMEVAL_H

#include <ulib/string.h>

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>
#endif

#include <time.h>

#define U_SECOND 1000000L

class U_EXPORT UTimeVal : public timeval {
public:

   // NB: l'oggetto puo' essere usato come (struct timeval) in quanto UMemoryError viene allocato dopo...

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   static void adjust(long* tv_sec, long* tv_usec);

   void adjust()
      {
      U_TRACE(0, "UTimeVal::adjust()")

      U_CHECK_MEMORY

      adjust((long*)&tv_sec, (long*)&tv_usec);

      U_INTERNAL_DUMP("this = %O", U_OBJECT_TO_TRACE(*this))
      }

   // COSTRUTTORI

   UTimeVal()
      {
      U_TRACE_REGISTER_OBJECT(0, UTimeVal, "")

      U_INTERNAL_DUMP("this         = %p", this)
      U_INTERNAL_DUMP("&tv_sec      = %p", &tv_sec)
      U_INTERNAL_DUMP("&tv_usec     = %p", &tv_usec)
      U_INTERNAL_DUMP("memory._this = %p", memory._this)

      U_INTERNAL_ASSERT_EQUALS((void*)this, (void*)&tv_sec)
      }

   UTimeVal(long sec, long micro_sec = 0L)
      {
      U_TRACE_REGISTER_OBJECT(0, UTimeVal, "%ld,%ld", sec, micro_sec)

      tv_sec  = sec;
      tv_usec = micro_sec;
      }

   UTimeVal(struct timeval ctime)
      {
      U_TRACE_REGISTER_OBJECT(0, UTimeVal, "%ld,%ld", ctime.tv_sec, ctime.tv_usec)

      tv_sec  = ctime.tv_sec;
      tv_usec = ctime.tv_usec;
      }

   ~UTimeVal()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UTimeVal)
      }

   // ASSEGNAZIONI

   void set(long sec, long micro_sec)
      { tv_sec = sec; tv_usec = micro_sec; }

   void set(const UTimeVal& t)
      { tv_sec  = t.tv_sec; tv_usec = t.tv_usec; }

   void set(const struct timeval& t)
      { tv_sec  = t.tv_sec; tv_usec = t.tv_usec; }

   UTimeVal(const UTimeVal& t)
      {
      U_TRACE_REGISTER_OBJECT(0, UTimeVal, "%O", U_OBJECT_TO_TRACE(t))

      U_MEMORY_TEST_COPY(t)

      set(t);
      }

   UTimeVal& operator=(const UTimeVal& t)
      {
      U_TRACE(0, "UTimeVal::operator=(%O)", U_OBJECT_TO_TRACE(t))

      U_MEMORY_TEST_COPY(t)

      set(t);

      return *this;
      }

   // VARIE

   bool isZero() const
      {
      U_TRACE(0, "UTimeVal::isZero()")

      bool result = (tv_sec  == 0L &&
                     tv_usec == 0L);

      U_RETURN(result);
      }

   bool isNegativ() const
      {
      U_TRACE(0,"UTimeVal::isNegativ()")

      bool result = (tv_sec < 0L || tv_usec < 0L);

      U_RETURN(result);
      }

   bool notZero() const    { return (isZero()    == false); }
   bool notNegativ() const { return (isNegativ() == false); }

   void add(long sec, long micro_sec)
      {
      U_TRACE(0, "UTimeVal::add(%ld,%ld)", sec, micro_sec)

      tv_sec  += sec;
      tv_usec += micro_sec;

      adjust();
      }

   void sub(long sec, long micro_sec)
      {
      U_TRACE(0, "UTimeVal::sub(%ld,%ld)", sec, micro_sec)

      tv_sec  -= sec;
      tv_usec -= micro_sec;

      adjust();
      }

   void setZero()                      { tv_sec = tv_usec = 0L; }
   void setSecond(long sec)            { tv_sec = sec; }
   void setMicroSecond(long micro_sec) { tv_usec = micro_sec; }

   long getSecond() const
      {
      U_TRACE(0, "UTimeVal::getSecond()")

      U_CHECK_MEMORY

      long result = tv_sec + (tv_usec >= 500000L ? 1 : 0);

      U_RETURN(result);
      }

   long getMilliSecond() const
      {
      U_TRACE(0, "UTimeVal::getMilliSecond()")

      U_CHECK_MEMORY

      long ms = tv_sec * 1000L + (tv_usec / 1000L);

      U_RETURN(ms);
      }

   double getMicroSecond() const
      {
      U_TRACE(0, "UTimeVal::getMicroSecond()")

      double micro_sec = (double) tv_sec + (tv_usec / 1000000.);

      U_RETURN(micro_sec);
      }

   // OPERATOR

   int operator !() const        { return isZero(); }
       operator timeval*() const { return (struct timeval*)this; }

   bool operator==(const UTimeVal& t) const
      {
      U_TRACE(0, "UTimeVal::operator==(%O)", U_OBJECT_TO_TRACE(t))

      U_CHECK_MEMORY

      bool result = (tv_sec == t.tv_sec && tv_usec == t.tv_usec);

      U_RETURN(result);
      }

   bool operator< (const UTimeVal& t) const __pure;
   bool operator> (const UTimeVal& t) const { return  t.operator<(*this); }
   bool operator<=(const UTimeVal& t) const { return !t.operator<(*this); }
   bool operator>=(const UTimeVal& t) const { return !  operator<(t); }
   bool operator!=(const UTimeVal& t) const { return !  operator==(t); }

   UTimeVal& operator+=(const UTimeVal& t)
      {
      U_TRACE(0, "UTimeVal::operator+=(%p)", &t)

      add(t.tv_sec, t.tv_usec);

      return *this;
      }

   UTimeVal& operator-=(const UTimeVal& t)
      {
      U_TRACE(0, "UTimeVal::operator-=(%p)", &t)

      sub(t.tv_sec, t.tv_usec);

      return *this;
      }

   friend UTimeVal operator+(const UTimeVal& t1, const UTimeVal& t2)
      {
      U_TRACE(0, "UTimeVal::operator+(%p,%p)", &t1, &t2)

      return UTimeVal(t1) += t2;
      }

   friend UTimeVal operator-(const UTimeVal& t1, const UTimeVal& t2)
      {
      U_TRACE(0, "UTimeVal::operator+(%p,%p)", &t1, &t2)

      return UTimeVal(t1) -= t2;
      }

   UTimeVal& operator+=(long micro_sec)
      {
      U_TRACE(0, "UTimeVal::operator+=(%ld)", micro_sec)

      add(micro_sec / U_SECOND, micro_sec % U_SECOND);

      return *this;
      }

   UTimeVal& operator-=(long micro_sec)
      {
      U_TRACE(0, "UTimeVal::operator-=(%ld)", micro_sec)

      sub(micro_sec / U_SECOND, micro_sec % U_SECOND);

      return *this;
      }

   friend UTimeVal operator+(const UTimeVal& t, long micro_sec)
      {
      U_TRACE(0, "UTimeVal::operator+(%p,%ld)", &t, micro_sec)

      return UTimeVal(t) += micro_sec;
      }

   friend UTimeVal operator-(const UTimeVal& t, long micro_sec)
      {
      U_TRACE(0, "UTimeVal::operator-(%p,%ld)", &t, micro_sec)

      return UTimeVal(t) -= micro_sec;
      }

   // TIMESPEC

   /*
   struct timespec {
      time_t tv_sec;  // seconds
      long   tv_nsec; // nanoseconds
      };
   */

   void setTimeSpec(struct timespec* t)
      {
      U_TRACE(0, "UTimeVal::setTimeSpec(%p)", t)

      U_CHECK_MEMORY

      U_INTERNAL_ASSERT_RANGE(0L, tv_usec, U_SECOND)

      t->tv_sec  = tv_sec;
      t->tv_nsec = tv_usec * 1000;
      }

   bool operator>(struct timespec* t) const
      {
      U_TRACE(0, "UTimeVal::operator>(%O,{%ld,%ld})", U_OBJECT_TO_TRACE(*this), t->tv_sec, t->tv_nsec)

      U_CHECK_MEMORY

      U_INTERNAL_ASSERT_RANGE(0L, tv_usec, U_SECOND)

      bool result = (tv_sec >  t->tv_sec ||
                    (tv_sec == t->tv_sec && ((tv_usec * 1000) > t->tv_nsec)));

      U_RETURN(result);
      }

   // SERVICES

   void nanosleep();

   // STREAM

   friend U_EXPORT istream& operator>>(istream& is,       UTimeVal& t);
   friend U_EXPORT ostream& operator<<(ostream& os, const UTimeVal& t);

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif
};

#endif
