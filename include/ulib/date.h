// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    date.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_DATE_H
#define ULIB_DATE_H

#include <ulib/string.h>

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>
#endif

#include <time.h>

/**
 * The UDate class uses a julian date representation of the current year, month, and day.
 * This is then manipulated in several forms and may be exported as needed
 */

#define FIRST_DAY   2361222  // Julian day for 1752/09/14
#define FIRST_YEAR  1752     // wrong for many countries

class U_EXPORT UDate {
public:

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   // COSTRUTTORI

   UDate()
      {
      U_TRACE_REGISTER_OBJECT(0, UDate, "")

      julian = _day = _month = _year = 0;
      }

   void fromTime(time_t tm);

   UDate(time_t tm)
      {
      U_TRACE_REGISTER_OBJECT(0, UDate, "%#4D", tm)

      fromTime(tm);
      }

   UDate(const char* str, bool UTC = false); // UTC is flag for date and time in Coordinated Universal Time: format YYMMDDMMSSZ

   void setYear(int year);

   void set(int day, int month, int year)
      {
      U_TRACE(0, "UDate::set(%d,%d,%d)", day, month, year)

      U_INTERNAL_ASSERT_RANGE(1,   day, 31)
      U_INTERNAL_ASSERT_RANGE(1, month, 12)

      julian = 0;
      _day   = day;
      _month = month;

      setYear(year);
      }

   // Construct a UDate with a given day of the year and a given year.
   // The base date for this computation is 31 Dec. of the previous year.
   // i.e., UDate(-1,1901) = 31 Dec. 1900 and UDate(1,1901) = 2 Jan. 1901

   UDate(int day,            int year);
   UDate(int day, int month, int year);

   ~UDate()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UDate)
      }

   // ASSEGNAZIONI

   void set(const UDate& d)
      {
      _day   = d._day;
      _month = d._month;
      _year  = d._year;
      julian = d.julian;
      }

   UDate(const UDate& d)
      {
      U_TRACE_REGISTER_OBJECT(0, UDate, "%O", U_OBJECT_TO_TRACE(d))

      U_MEMORY_TEST_COPY(d)

      set(d);
      }

   UDate& operator=(const UDate& d)
      {
      U_TRACE(0, "UDate::operator=(%O)", U_OBJECT_TO_TRACE(d))

      U_MEMORY_TEST_COPY(d)

      set(d);

      return *this;
      }

   // SERVICES

   bool isValid() const;

   int getJulian();

   int getDay() const   { return _day; }
   int getYear() const  { return _year; }
   int getMonth() const { return _month; }

   // The daysTo() function returns the number of days between two date

   int daysTo(UDate& date)
      {
      U_TRACE(0, "UDate::daysTo(%p)", &date)

      U_RETURN(date.getJulian() - getJulian());
      }

   void addDays(int days)
      {
      U_TRACE(0, "UDate::addDays(%d)", days)

      julian = getJulian() + days;

      fromJulian(julian);
      }

   void addMonths(int months);

   static int getMonth(const char* buf) __pure;

   void addYears(int years)
      {
      U_TRACE(0, "UDate::addYears(%d)", years)

      _year += years;

      julian = toJulian(_day, _month, _year);
      }

   int getDayOfWeek()
      {
      U_TRACE(0, "UDate::getDayOfWeek()")

      // (julian + 1) % 7 gives the value 0 for Sunday ... 6 for Saturday

      U_RETURN((getJulian() + 1) % 7);
      }

   // Returns the number of days in the month (28..31) for this date

   int getDaysInMonth() const;

   // Returns the day of the year [1,366] for this date

   int getDayOfYear() __pure;

   // Returns true if the specified year is a leap year; otherwise returns false

   static bool leapYear(int y);

   // print date with format

          UString strftime(const char* fmt);
   static UString strftime(const char* fmt, time_t t);

   static time_t getSecondFromJulian(int _julian)
      {
      U_TRACE(0, "UDate::getSecondFromJulian(%d)", _julian)

      U_INTERNAL_ASSERT(_julian >= 2440588) // 2440588 -> 01/01/1970

      time_t t = U_ONE_DAY_IN_SECOND * (_julian - 2440588);

      U_RETURN(t);
      }

   time_t getSecond()
      {
      U_TRACE(0, "UDate::getSecond()")

      time_t t = getSecondFromJulian(getJulian());

      U_RETURN(t);
      }

   static time_t getSecondFromTime(const char* str) // seconds from string in HH:MM:SS format
      {
      U_TRACE(0, "UDate::getSecondFromTime(%.*S)", 8, str)

      U_INTERNAL_ASSERT_EQUALS(str[2], ':')
      U_INTERNAL_ASSERT_EQUALS(str[5], ':')

      // NB: sscanf() is VERY HEAVY...!!!

      time_t t = (60 * 60 * atoi(str)) + (60 * atoi(str+3)) + atoi(str+6);

      U_RETURN(t);
      }

   static time_t getSecondFromTime(const char* str, bool gmt, const char* fmt = "%a, %d %b %Y %H:%M:%S GMT", struct tm* tm = 0);

   void setCurrentDate()
      {
      U_TRACE(0, "UDate::setCurrentDate()")

      // UNIX system time - SecsSince1Jan1970UTC

      u_gettimeofday();

      fromTime(u_now->tv_sec);
      }

   static void setCurrentDate(UDate& date)
      {
      U_TRACE(0, "UDate::setCurrentDate(%p)", &date)

      date.setCurrentDate();
      }

   // OPERATOR

   bool operator==(UDate& date);
   bool operator!=(UDate& date);
   bool operator< (UDate& date);
   bool operator<=(UDate& date);
   bool operator> (UDate& date);
   bool operator>=(UDate& date);

   UDate& operator+=(UDate& d)
      {
      U_TRACE(0, "UDate::operator+=(%p)", &d)

      julian = getJulian() + d.getJulian();

      fromJulian(julian);

      return *this;
      }

   UDate& operator-=(UDate& d)
      {
      U_TRACE(0, "UDate::operator-=(%p)", &d)

      julian = getJulian() - d.getJulian();

      fromJulian(julian);

      return *this;
      }

   friend UDate operator+(const UDate& d1, UDate& d2)
      {
      U_TRACE(0, "UDate::operator+(%p,%p)", &d1, &d2)

      return UDate(d1) += d2;
      }

   friend UDate operator-(const UDate& d1, UDate& d2)
      {
      U_TRACE(0, "UDate::operator+(%p,%p)", &d1, &d2)

      return UDate(d1) -= d2;
      }

   UDate& operator+=(int days)
      {
      U_TRACE(0, "UDate::operator+=(%d)", days)

      addDays(days);

      return *this;
      }

   UDate& operator-=(int days)
      {
      U_TRACE(0, "UDate::operator-=(%d)", days)

      addDays(-days);

      return *this;
      }

   friend UDate operator+(UDate& d, int days)
      {
      U_TRACE(0, "UDate::operator+(%p,%d)", &d, days)

      return UDate(d) += days;
      }

   friend UDate operator-(UDate& d, int days)
      {
      U_TRACE(0, "UDate::operator-(%p,%d)", &d, days)

      return UDate(d) -= days;
      }

   // STREAM

   friend U_EXPORT istream& operator>>(istream& is,       UDate& d); // dd/mm/yyyy
   friend U_EXPORT ostream& operator<<(ostream& os, const UDate& d); // dd/mm/yyyy

#ifdef DEBUG
   bool invariant() { return (julian >= FIRST_DAY); } // 14/09/1752

   const char* dump(bool reset) const;
#endif

protected:
   int julian, _day, _month, _year;

   void fromJulian(int julian);

   static int toJulian(int day, int month, int year);
};

#endif
