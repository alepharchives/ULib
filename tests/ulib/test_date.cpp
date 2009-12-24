// test_date.cpp

#include <ulib/date.h>

static UDate data1(31,12,99);
static UDate data2("31/12/99");

int
U_EXPORT main (int argc, char* argv[])
{
   U_ULIB_INIT(argv);

   U_TRACE(5,"main(%d)",argc)

   U_ASSERT( UDate("14/09/1752").getJulian() == 2361222 )
   U_ASSERT( UDate("31/12/1900").getJulian() == 2415385 )
   U_ASSERT( UDate("01/01/1970").getJulian() == 2440588 )

   U_ASSERT( data1 == data2 )
   U_ASSERT( data1.getDayOfWeek() == 5 )           // Venerdi
   U_ASSERT( data2.getDayOfYear() == 365 )

   U_ASSERT( UDate("1/3/00").getDayOfWeek() == 3 ) // Mercoledi
   U_ASSERT( UDate(31,12,00).getDayOfYear() == 366 )

   UDate data3(60,2000);
   UDate data4("29/02/00");

   U_ASSERT( data3 == data4 )
   U_ASSERT( data3.getDayOfYear() == 60 )

   UDate data5(60,1901);
   UDate data6("1/3/1901");

   U_ASSERT( data5 == data6 )

   U_ASSERT( UDate(17, 5, 2002).isValid() == true )  // TRUE   May 17th 2002 is valid
   U_ASSERT( UDate(30, 2, 2002).isValid() == false ) // FALSE  Feb 30th does not exist
   U_ASSERT( UDate(29, 2, 2004).isValid() == true )  // TRUE   2004 is a leap year

   U_ASSERT( UDate(29, 2, 2004).strftime("%Y-%m-%d") == U_STRING_FROM_CONSTANT("2004-02-29") )

   U_ASSERT( UDate("14/09/1752").getJulian() == 2361222 )

   cout << "Date: " << data6.strftime("%d/%m/%y") << '\n';

   while (cin >> data6) cout << data6 << '\n';
}
