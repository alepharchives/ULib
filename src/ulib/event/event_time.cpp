// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//   event_time.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/event/event_time.h>

void UEventTime::setCurrentTime()
{
   U_TRACE(1, "UEventTime::setCurrentTime()")

   U_CHECK_MEMORY

   (void) U_SYSCALL(gettimeofday, "%p,%p", &u_now, 0);

   U_INTERNAL_DUMP("u_now = { %ld %6ld }", u_now.tv_sec, u_now.tv_usec)

   ctime = u_now;
}

void UEventTime::setTimerVal(struct timeval* it_value)
{
   U_TRACE(0, "UEventTime::setTimerVal(%p)", it_value)

   U_CHECK_MEMORY

   it_value->tv_sec  = ctime.tv_sec  + tv_sec  - u_now.tv_sec;
   it_value->tv_usec = ctime.tv_usec + tv_usec - u_now.tv_usec;

   UTimeVal::adjust((long*)&(it_value->tv_sec), (long*)&(it_value->tv_usec));

   U_INTERNAL_DUMP("it_value = { %ld %6ld }", it_value->tv_sec, it_value->tv_usec)

   U_INTERNAL_ASSERT(it_value->tv_sec  >= 0)
   U_INTERNAL_ASSERT(it_value->tv_usec >= 0)
}

bool UEventTime::isOld() const
{
   U_TRACE(0, "UEventTime::isOld()")

   U_CHECK_MEMORY

   long t1 = (ctime.tv_sec + tv_sec);

   U_INTERNAL_DUMP("this = { %ld %6ld }", t1, ctime.tv_usec + tv_usec)

   bool result = (  t1  < u_now.tv_sec) ||
                  ((t1 == u_now.tv_sec) &&
                   ((ctime.tv_usec + tv_usec) < u_now.tv_usec));

   U_RETURN(result);
}

bool UEventTime::operator<(const UEventTime& t) const
{
   U_TRACE(0, "UEventTime::operator<(%O)", U_OBJECT_TO_TRACE(t))

   long t1 = (  ctime.tv_sec +   tv_sec),
        t2 = (t.ctime.tv_sec + t.tv_sec);

   U_INTERNAL_DUMP("{ %ld %6ld } < { %ld %6ld }", t1,   ctime.tv_usec +   tv_usec,
                                                  t2, t.ctime.tv_usec + t.tv_usec)

   bool result = (  t1 <  t2) ||
                  ((t1 == t2) &&
                   ((ctime.tv_usec + tv_usec) < (t.ctime.tv_usec + t.tv_usec)));

   U_INTERNAL_DUMP("result = %b", result)

   return result;
}

// STREAM

U_EXPORT ostream& operator<<(ostream& os, const UEventTime& t)
{
   U_TRACE(0, "UEventTime::operator<<(%p,%p)", &os, &t)

   os.put('{');
   os.put(' ');
   os << t.ctime.tv_sec;
   os.put(' ');
   os.width(6);
   os << t.ctime.tv_usec;
   os.put(' ');
   os.put('{');
   os.put(' ');
   os << t.tv_sec;
   os.put(' ');
   os.width(6);
   os << t.tv_usec;
   os.put(' ');
   os.put('}');
   os.put(' ');
   os.put('}');

   return os;
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UEventTime::dump(bool reset) const
{
   UTimeVal::dump(false);

   *UObjectIO::os << '\n'
                  << "ctime   " << "{ " << ctime.tv_sec
                                << " "  << ctime.tv_usec
                                << " }";

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
