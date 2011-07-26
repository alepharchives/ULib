// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    date.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/date.h>

static const short monthDays[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
static const char* months[]    = { "jan", "feb", "mar", "apr", "may", "jun", "jul", "aug", "sep", "oct", "nov", "dec" };
static const char* months_it[] = { "gen", "feb", "mar", "apr", "mag", "giu", "lug", "ago", "set", "ott", "nov", "dic" };

UDate::UDate(int day, int year)
{
   U_TRACE_REGISTER_OBJECT(0, UDate, "%d,%d", day, year)

   julian = toJulian(1,1,year) - 1 + day;

   fromJulian(julian);
}

UDate::UDate(int day, int month, int year)
{
   U_TRACE_REGISTER_OBJECT(0, UDate, "%d,%d,%d", day, month, year)

   set(day, month, year);
}

__pure int UDate::getDayOfYear()
{
   U_TRACE(0, "UDate::getDayOfYear()")

   int y = _year - 1901;

   U_RETURN(getJulian() - (y * 365) - (y / 4) - 2415385); // 2415385 -> 31/12/1900
}

__pure int UDate::getMonth(const char* buf)
{
   U_TRACE(0, "UDate::getMonth(%S)", buf)

   const char* ptr;

   for (int i = 0; i < 12; ++i)
      {
      ptr = months[i];

      if (((buf[0] == ptr[0]) || (buf[0] == ptr[0] - 32)) &&
          ((buf[1] == ptr[1]) || (buf[1] == ptr[1] - 32)) &&
          ((buf[2] == ptr[2]) || (buf[2] == ptr[2] - 32)))
         {
         return i+1;
         }

      ptr = months_it[i];

      if (((buf[0] == ptr[0]) || (buf[0] == ptr[0] - 32)) &&
          ((buf[1] == ptr[1]) || (buf[1] == ptr[1] - 32)) &&
          ((buf[2] == ptr[2]) || (buf[2] == ptr[2] - 32)))
         {
         return i+1;
         }
      }

   return 0;
}

bool UDate::leapYear(int y)
{
   U_TRACE(0, "UDate::leapYear(%d)", y)

   // 1. Every year that is divisible by four is a leap year;
   // 2. of those years, if it can be divided by 100, it is NOT a leap year, unless
   // 3. the year is divisible by 400. Then it is a leap year.

   bool result =  ((y %   4) == 0) &&
                 (((y % 100) != 0) ||
                  ((y % 400) == 0));

   U_RETURN(result);
}

__pure bool UDate::isValid() const
{
   U_TRACE(0, "UDate::isValid()")

   U_CHECK_MEMORY

   if ( _year < FIRST_YEAR || (_year == FIRST_YEAR && (_month < 9 || (_month == 9 && _day < 14))) ) U_RETURN(false);

   bool result = (_day > 0 && _month > 0 && _month <= 12) &&
                 (_day <= monthDays[_month] || (_day == 29 && _month == 2 && leapYear(_year)));

   U_RETURN(result);
}

__pure int UDate::getDaysInMonth() const
{
   U_TRACE(0, "UDate::getDaysInMonth()")

   if (_month == 2 && leapYear(_year)) U_RETURN(29);

   U_RETURN(monthDays[_month]);
}

int UDate::toJulian(int day, int month, int year)
{
   U_TRACE(0, "UDate::toJulian(%d,%d,%d)", day, month, year)

   U_INTERNAL_ASSERT_RANGE(1,     day,   31)
   U_INTERNAL_ASSERT_RANGE(1,   month,   12)
   U_INTERNAL_ASSERT_RANGE(1752, year, 8000)

   int _julian;

   /*
   _julian = day - 32075l +
            1461l * (year  + 4800l + (month - 14l) / 12l) /   4l +
             367l * (month - 2l    - (month - 14l) / 12l  *  12l) / 12l -
               3l * ((year + 4900l + (month - 14l) / 12l) / 100l) / 4l;

   U_INTERNAL_DUMP("_julian = %d", _julian)
   */

   // -----------------------------------------------------------------------
   // algorithm 199 from Communications of the ACM, Volume 6, No. 8
   // (Aug. 1963), p. 444. Gregorian calendar started on 14 Sep 1752.
   // -----------------------------------------------------------------------
   // The UDate is based on the Gregorian (modern western) calendar. England
   // adopted the Gregorian calendar on September 14th 1752, which is the
   // earliest date that is supported by UDate. Using earlier dates will give
   // undefined results. Some countries adopted the Gregorian calendar later
   // than England, thus the week day of early dates might be incorrect for
   // these countries (but correct for England). The end of time is reached
   // around 8000AD, by which time we expect UDate to be obsolete.
   // -----------------------------------------------------------------------

   month += (month > 2 ? -3 : (--year, 9));

   int century = year / 100;

   year -= century * 100;

   _julian = 1721119 + day + ((146097 * century) / 4) +
                             ((  1461 * year)    / 4) +
                             ((   153 * month) + 2) / 5;

   U_INTERNAL_ASSERT(_julian >= FIRST_DAY)

   U_RETURN(_julian);
}

// gcc: call is unlikely and code size would grow

int UDate::getJulian() { return (julian ? julian : julian = toJulian(_day, _month, _year)); }

bool UDate::operator==(UDate& date) { return getJulian() == date.getJulian(); }
bool UDate::operator!=(UDate& date) { return getJulian() != date.getJulian(); }
bool UDate::operator< (UDate& date) { return getJulian() <  date.getJulian(); }
bool UDate::operator<=(UDate& date) { return getJulian() <= date.getJulian(); }
bool UDate::operator> (UDate& date) { return getJulian() >  date.getJulian(); }
bool UDate::operator>=(UDate& date) { return getJulian() >= date.getJulian(); }

void UDate::fromJulian(int j)
{
   U_TRACE(0, "UDate::fromJulian(%d)", j)

   /*
   int t1, t2;

   t1    = j + 32075;
   _year = 4 * t1 / 1461;

   t1     = t1 - 1461 * _year / 4;
   t2     = 3 * (_year + 100) / 400;
   t1     = t1 + t2;
   _month = 12 * t1 / 367;
   t2     = _month / 11;

   _day    = t1 - 367 * _month / 12;
   _month = _month + 2 - 12 * t2;
   _year  = _year - 4800 + t2;

   U_INTERNAL_DUMP("_day = %d, _month = %d, _year = %d", _day, _month, _year)
   */

   j      -= 1721119;
   _year   = ((j * 4) - 1) /  146097;
   j       =  (j * 4) - 1  - (146097 * _year);
   _day    =   j / 4;
   j       = ((_day * 4) + 3) /  1461;
   _day    =  (_day * 4) + 3  - (1461 * j);
   _day    =  (_day + 4) / 4;
   _month  = (5 * _day - 3) / 153;
   _day    =  5 * _day - 3  - 153 * _month;
   _day    = (_day + 5) / 5;
   _year   = 100 * _year + j;
   _month += (_month < 10 ? 3 : (++_year, -9));

   U_INTERNAL_DUMP("_day = %d, _month = %d, _year = %d", _day, _month, _year)

   U_INTERNAL_ASSERT_RANGE(1,       _day,  31)
   U_INTERNAL_ASSERT_RANGE(1,     _month,  12)
   U_INTERNAL_ASSERT_RANGE(1752, _year, 8000)

   U_INTERNAL_ASSERT(isValid())
}

void UDate::fromTime(time_t tm)
{
   U_TRACE(1, "UDate::fromTime(%#4D)", tm)

   if (tm)
      {
#  if defined(DEBUG) && !defined(__MINGW32__)
      U_SYSCALL_VOID(localtime_r, "%p,%p", &tm, &u_strftime_tm);
#  else
                     localtime_r(          &tm, &u_strftime_tm);
#  endif

      _day   = u_strftime_tm.tm_mday;
      _month = u_strftime_tm.tm_mon  + 1;
      _year  = u_strftime_tm.tm_year + 1900;
      julian = 0;
      }
   else
      {
      _day   =
      _month = 1;
      _year  = 1970;
      julian = 2440588;
      }
}

void UDate::setYear(int year)
{
   U_TRACE(0, "UDate::setYear(%d)", year)

   // [   0,  50) -> [2000,2050)
   // [  50, 150] -> [1950,2050)
   // [1752,8000] -> [1752,8000]

   _year = year;

   if (year < 150) _year += 1900;
   if (year <  50) _year +=  100;

   U_INTERNAL_ASSERT_RANGE(1752, _year, 8000)

   if (julian) julian = toJulian(_day, _month, _year);
}

// UTC is flag for date and time in Coordinated Universal Time : format YYMMDDMMSSZ

UDate::UDate(const char* str, bool UTC)
{
   U_TRACE_REGISTER_OBJECT(0, UDate, "%S,%b", str, UTC)

   julian = _day = _month = _year = 0;

   int year;
   const char* format = (UTC ? "%02u%02u%02u" : "%d/%d/%d");
   int scanned = (str && *str ? U_SYSCALL(sscanf, "%S,%S,%p,%p,%p", str, format, &_day, &_month, &year) : 0);

   if      (scanned == 0) return; 
   else if (scanned == 3) setYear(year);
   else
      {
      // Complete for the user

      u_gettimeofday();

#  if defined(DEBUG) && !defined(__MINGW32__)
      U_SYSCALL_VOID(localtime_r, "%p,%p", &(u_now->tv_sec), &u_strftime_tm);
#  else
                     localtime_r(          &(u_now->tv_sec), &u_strftime_tm);
#  endif

      switch (scanned)
         {
      // case 0: _day   = u_strftime_tm.tm_mday;
         case 1: _month = u_strftime_tm.tm_mon  + 1;
         case 2: _year  = u_strftime_tm.tm_year + 1900;
         }
      }
}

UString UDate::strftime(const char* fmt)
{
   U_TRACE(0, "UDate::strftime(%S)", fmt)

   UString result(100U);

   (void) memset(&u_strftime_tm, 0, sizeof(struct tm));

   u_strftime_tm.tm_mday = _day;
   u_strftime_tm.tm_mon  = _month - 1;
   u_strftime_tm.tm_year = _year  - 1900;

   uint32_t length = u_strftime(result.data(), result.capacity(), fmt, -1);

   result.size_adjust(length);

   U_RETURN_STRING(result);
}

UString UDate::strftime(const char* fmt, time_t t)
{
   U_TRACE(0, "UDate::strftime(%S,%ld)", fmt, t)

   UString result(100U);

   (void) memset(&u_strftime_tm, 0, sizeof(struct tm));

   uint32_t length = u_strftime(result.data(), result.capacity(), fmt, t);

   result.size_adjust(length);

   U_RETURN_STRING(result);
}

time_t UDate::getSecondFromTime(const char* str, bool gmt, const char* fmt, struct tm* tm)
{
   U_TRACE(1, "UDate::getSecondFromTime(%S,%b,%S,%p)", str, gmt, fmt, tm)

   if (tm == 0)
      {
      static struct tm tm_;

      tm = &tm_;
      }

   (void) memset(tm, 0, sizeof(struct tm)); // do not remove this

   time_t t;

   if (gmt)
      {
      if (str[3] == ',')
         {
         /* Fri, 31 Dec 1999 23:59:59 GMT
          * |  | |  |   |    |  |  | 
          * 0  3 5  8  12   17 20 23
          */

         tm->tm_mday = atoi(str+5);
         tm->tm_mon  = getMonth(str+8);
         tm->tm_year = atoi(str+12);
         tm->tm_hour = atoi(str+17);
         tm->tm_min  = atoi(str+20);
         tm->tm_sec  = atoi(str+23);
         }
      else if ((tm->tm_mon = getMonth(str)))
         {
         /* Jan 25 11:54:00 2005 GMT
          * |   |  |  |  |  |
          * 0   4  7 10 13 16
          */

         tm->tm_mday = atoi(str+4);
         tm->tm_hour = atoi(str+7);
         tm->tm_min  = atoi(str+10);
         tm->tm_sec  = atoi(str+13);
         tm->tm_year = atoi(str+16);
         }
      else
         {
         goto scanf;
         }
      }
   else
      {
scanf:
      // NB: fmt must be compatible with the sequence "YY MM DD HH MM SS"...

      int n = U_SYSCALL(sscanf, "%S,%S,%p,%p,%p,%p,%p,%p", str, fmt,
                        &tm->tm_year, &tm->tm_mon, &tm->tm_mday, &tm->tm_hour, &tm->tm_min, &tm->tm_sec);

      if (n != 6) U_RETURN(-1);

      // ts.tm_year is number of years since 1900

      if      (tm->tm_year <  50) { tm->tm_year += 2000; }
      else if (tm->tm_year < 100) { tm->tm_year += 1900; }
      }

   if ((tm->tm_year < 1900) ||
       (tm->tm_mon  < 1)    || (tm->tm_mon  > 12) ||
       (tm->tm_mday < 1)    || (tm->tm_mday > 31) ||
       (tm->tm_hour < 0)    || (tm->tm_hour > 23) ||
       (tm->tm_min  < 0)    || (tm->tm_min  > 59) ||
       (tm->tm_sec  < 0)    || (tm->tm_sec  > 61))
      {
      U_RETURN(-1);
      }

   if (gmt)
      {
      int _julian = toJulian(tm->tm_mday, tm->tm_mon, tm->tm_year);

      t = tm->tm_sec + (tm->tm_min * 60) + (tm->tm_hour * 3600) + getSecondFromJulian(_julian);

#  if defined(DEBUG) && !defined(__MINGW32__)
      tm->tm_year -= 1900; /* tm relative format year  - is number of years since 1900 */
      tm->tm_mon  -=    1; /* tm relative format month - range from 0-11 */

      U_INTERNAL_ASSERT_EQUALS(t, timegm(tm))
#  endif
      }
   else
      {
      /* NB: The timelocal() function is equivalent to the POSIX standard function mktime(3) */

      tm->tm_year -= 1900; /* tm relative format year  - is number of years since 1900 */
      tm->tm_mon  -=    1; /* tm relative format month - range from 0-11 */
      tm->tm_isdst =   -1;

      t = U_SYSCALL(mktime, "%p", tm);
      }

   U_RETURN(t);
}

void UDate::addMonths(int nmonths)
{
   U_TRACE(0, "UDate::addMonths(%d)", nmonths)

   while (nmonths != 0)
      {
      if (nmonths      <  0 &&
          nmonths + 12 <= 0)
         {
         --_year;

         nmonths += 12;
         }
      else if (nmonths < 0)
         {
         _month += nmonths;
                   nmonths = 0;

         if (_month <= 0)
            {
            --_year;

            _month += 12;
            }
         }
      else if (nmonths - 12 >= 0)
         {
         ++_year;

         nmonths -= 12;
         }
      else if (_month == 12)
         {
         ++_year;

         _month = 0;
         }
      else
         {
         _month += nmonths;

         nmonths = 0;

         if (_month > 12)
            {
            ++_year;

            _month -= 12;
            }
         }
      }

   int days_in_month = (_month == 2 && leapYear(_year) ? 29 : monthDays[_month]);

   if (_day > days_in_month) _day = days_in_month;
}

// STREAM

U_EXPORT istream& operator>>(istream& is, UDate& d)
{
   U_TRACE(0, "UDate::operator>>(%p,%p)", &is, &d)

   streambuf* sb = is.rdbuf();

   is >> d._day;

   sb->sbumpc(); // skip '/'

   is >> d._month;

   sb->sbumpc(); // skip '/'

   is >> d._year;

   d.julian = 0;

   return is;
}

U_EXPORT ostream& operator<<(ostream& os, const UDate& d)
{
   U_TRACE(0, "UDate::operator<<(%p,%p)", &os, &d)

   os << d._day   << '/'
      << d._month << '/'
      << d._year;

   return os;
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UDate::dump(bool reset) const
{
   *UObjectIO::os << "_day    " << _day   << '\n'
                  << "_month  " << _month << '\n'
                  << "_year   " << _year  << '\n'
                  << "julian  " << julian;

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
