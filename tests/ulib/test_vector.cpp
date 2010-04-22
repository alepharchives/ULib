// test_vector.cpp

#include <ulib/file.h>
#include <ulib/base/hash.h>

#define U_RING_BUFFER
#include <ulib/container/vector.h>

#ifdef HAVE_STRSTREAM_H
#  include <strstream.h>
#else
#  include <ulib/replace/strstream.h>
#endif

#undef min
using std::min;
#include <fstream>

class Product {
public:
    Product() {}
   ~Product() { cout << "\ndistruttore Product\n"; }
};

// how to override the default...

template <>
inline void u_destroy(Product** ptr, uint32_t n)
{
   U_TRACE(0, "u_destroy<Product>(%p,%u)", ptr, n)
}

/*
NB: used by method
void assign(unsigned n, T* elem))
void erase(unsigned first, unsigned last)
*/

static void check_vector_destructor()
{
   U_TRACE(5, "check_vector_destructor()")

   UVector<Product*> z;

   z.push_back(new Product);
   z.push_back(new Product);
}

static void check(UVector<UString>& y)
{
   U_TRACE(5,"check()")

   unsigned n;
   UString tmp;

/* input
[
ROOT_DN                       0
PASSWORD                      1
ROOT_DN_MAIL                  2
PASSWORD_MAIL                 3
CHECK_QUOTING                 4
LDAP_SERVER_ADDRESS           5
LOG_FILE                      6
LDAP_SERVER_ADDRESS_MAIL      7
ADMIN_DN                      8
ADMIN_DN_MAIL                 9
TIME_SLEEP_LDAP_ERROR         10
TIME_SLEEP_MQSERIES_ERROR     11
FILE_WRONG_MESSAGE            12
MAILDELIVERYOPTION            13
MESSAGE_QUEUE_SERVER          14
MESSAGE_QUEUE_MANAGER         15
MAILHOST                      16
MESSAGE_QUEUE_NAME            17
MAX_ERROR_FOR_CONNECT         18
]
*/

   tmp = y.front();
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("ROOT_DN") )
   tmp = y.rend();
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("PASSWORD") )
   tmp = y.back();
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("MAX_ERROR_FOR_CONNECT") )

   tmp = y[10];
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("TIME_SLEEP_LDAP_ERROR") )
   tmp = y[11];
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("TIME_SLEEP_MQSERIES_ERROR") )
   tmp = y[12];
   U_ASSERT( tmp != U_STRING_FROM_CONSTANT("ROOT_DN") )

   n = y.find(U_STRING_FROM_CONSTANT("NULL"));
   U_ASSERT( n == unsigned(-1) )
   n = y.find(U_STRING_FROM_CONSTANT("ROOT_DN"));
   U_ASSERT( n == unsigned(0) )
   n = y.find(U_STRING_FROM_CONSTANT("MAX_ERROR_FOR_CONNECT"));
   U_ASSERT( n == unsigned(18) )

   y.pop();
   tmp = y.back();
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("MESSAGE_QUEUE_NAME") )

   y.push_back( U_STRING_FROM_CONSTANT("MAX_ERROR_FOR_CONNECT"));
   tmp = y.back();
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("MAX_ERROR_FOR_CONNECT") )

   y.insert(10, U_STRING_FROM_CONSTANT("NOT_PRESENT"));
   tmp = y[10];
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("NOT_PRESENT") )
   tmp = y[11];
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("TIME_SLEEP_LDAP_ERROR") )
   tmp = y[12];
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("TIME_SLEEP_MQSERIES_ERROR") )

   y.insert(10, 2,  U_STRING_FROM_CONSTANT("NOT_PRESENT"));
   tmp = y[10];
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("NOT_PRESENT") )
   tmp = y[11];
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("NOT_PRESENT") )
   tmp = y[12];
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("NOT_PRESENT") )
   tmp = y[13];
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("TIME_SLEEP_LDAP_ERROR") )
   tmp = y[14];
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("TIME_SLEEP_MQSERIES_ERROR") )

   y.erase(10);
   tmp = y[10];
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("NOT_PRESENT") )
   tmp = y[11];
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("NOT_PRESENT") )
   tmp = y[12];
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("TIME_SLEEP_LDAP_ERROR") )
   tmp = y[13];
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("TIME_SLEEP_MQSERIES_ERROR") )

   y.erase(10, 12);
   tmp = y[10];
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("TIME_SLEEP_LDAP_ERROR") )
   tmp = y[11];
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("TIME_SLEEP_MQSERIES_ERROR") )

   y.insert(19,    U_STRING_FROM_CONSTANT("NOT_PRESENT_1"));
   y.insert(20, 2, U_STRING_FROM_CONSTANT("NOT_PRESENT_2"));
   tmp = y[19];
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("NOT_PRESENT_1") )
   tmp = y[20];
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("NOT_PRESENT_2") )
   tmp = y[21];
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("NOT_PRESENT_2") )
   tmp = y[18];
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("MAX_ERROR_FOR_CONNECT") )

   y.erase(19, 22);
   U_ASSERT( y.back() == U_STRING_FROM_CONSTANT("MAX_ERROR_FOR_CONNECT") )

   y.assign(19, U_STRING_FROM_CONSTANT("NULL"));
   tmp = y.front();
   U_ASSERT( tmp ==U_STRING_FROM_CONSTANT("NULL") )
   tmp = y.back();
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("NULL") )
   tmp = y[17];
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("NULL") )
   tmp = y[18];
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("NULL") )

   y.reserve(y.capacity() * 2);
   y.clear();
   U_ASSERT( y.empty() == true )
}

static void print(UVector<UString>& y)
{
   U_TRACE(5, "print()")

   UString buffer(U_CAPACITY);
   unsigned size = 0, start = 0;

   char* ptr = buffer.data()     + start;
   size      = buffer.capacity() - start;

   ostrstream os(ptr, size);

   os << y;

   unsigned output_len = os.pcount();

   U_INTERNAL_DUMP("output_len = %d", output_len)

   U_INTERNAL_ASSERT_MINOR(output_len,size)

   buffer.size_adjust(start + output_len);

   cout << buffer << endl;
}

static int compareObj(const void* obj1, const void* obj2)
{
   return (*(UStringRep**)obj1)->compare(*(const UStringRep**)obj2);
}

static void check_contains()
{
   U_TRACE(5,"check_contains()")

   UString a0 = U_STRING_FROM_CONSTANT("accettazione, non-accettazione, avvenuta-consegna, rilevazione-virus"),
           b0 = U_STRING_FROM_CONSTANT("Consegna Virus");

   UVector<UString> a(a0, ", "), b(b0);

   U_ASSERT( a.contains(b)       == false )
   U_ASSERT( a.contains(b, true) == true )

   U_ASSERT( b.isContained(U_STRING_FROM_CONSTANT("virus"), true) == true )
}

static void check_equal()
{
   U_TRACE(5,"check_equal()")

   UString a0 = U_STRING_FROM_CONSTANT("accettazione, non-accettazione, avvenuta-consegna, rilevazione-virus"),
           b0 = U_STRING_FROM_CONSTANT("Avvenuta-Consegna rilevazione-Virus accettazione non-accettazione");

   UVector<UString> a(a0, ", "), b(b0);

   U_ASSERT( a.isEqual(b)       == false )
   U_ASSERT( a.isEqual(b, true) == true )

   U_ASSERT( b.find(U_STRING_FROM_CONSTANT("rilevazione-virus"), true) == true )
}

int
U_EXPORT main (int argc, char* argv[])
{
   U_ULIB_INIT(argv);

   U_TRACE(5,"main(%d)",argc)

   UString tmp = UFile::contentOf(argv[1]);
   UVector<UString> y(tmp);
   y.sort(false);

   uint32_t n, i = y.findSorted(U_STRING_FROM_CONSTANT("NULL"));
   U_ASSERT( i == U_NOT_FOUND )
   /*
   i = y.findSorted(U_STRING_FROM_CONSTANT("10sne1"));
   U_ASSERT( i == 12 )
   i = y.findSorted(U_STRING_FROM_CONSTANT("dakota"));
   U_ASSERT( i == 917 )
   i = y.findSorted(U_STRING_FROM_CONSTANT("victoria"));
   U_ASSERT( i == 2955 )
   i = y.findSorted(U_STRING_FROM_CONSTANT("zzz"));
   U_ASSERT( i == 3105 )
   i = y.findSorted(U_STRING_FROM_CONSTANT("!@#$%"));
   U_ASSERT( i == 0 )
   */

   for (i = 0, n = y.size(); i < n; ++i) { U_ASSERT( i == y.findSorted(y[i]) ) }

   ofstream outf("vector.sort");

   outf << y;

   y.clear();

   check_vector_destructor();
   check_contains();
   check_equal();

   cin >> y;

   print(y);
   check(y);

   // EXTENSION

   UString filter = U_STRING_FROM_CONSTANT("?db.*");
   n = UFile::listContentOf(y, 0, &filter);

   U_ASSERT( n == 2 )

   U_ASSERT( y[0] == U_STRING_FROM_CONSTANT("./cdb.test") ||
             y[0] == U_STRING_FROM_CONSTANT("./rdb.test"))
   U_ASSERT( y[1] == U_STRING_FROM_CONSTANT("./rdb.test") ||
             y[1] == U_STRING_FROM_CONSTANT("./cdb.test"))

   y.clear();
   bool res = y.empty();
   U_ASSERT( res == true )

   /*
   UString key(U_CAPACITY);
   unsigned i, j, k = 0;
// char ctmp[] = "                                                    "; // sizeof(tmp) == 53
   char ctmp[53];
   memset(ctmp, 'a', sizeof(ctmp)-1);
   ctmp[52] = '\0';

   for (i = 1; i < sizeof(ctmp) - 1; ++i)
      {
      for (j = 35; j < 122; ++j)
         {
         ctmp[i] = j;

         key.replace(ctmp, sizeof(ctmp));

         y.push_back(key);

         k = u_random(k);

         ctmp[1 + k % (sizeof(ctmp)-1)] = 35 + k % (122 - 35);

         key.replace(ctmp, sizeof(ctmp));

         y.push_back(key);
         }
      }

   y.sort(false);

   n = y.findSorted(U_STRING_FROM_CONSTANT("NULL"));
   U_ASSERT( n == U_NOT_FOUND )
   n = y.findSorted(U_STRING_FROM_CONSTANT("a$n5;FyAsX*;UGu(T0\\UaxwbV\%el4wZ,QeVqg3L\\],_O&---3PXg#"));
   U_ASSERT( n == 4 )
   n = y.findSorted(U_STRING_FROM_CONSTANT("axc`)dX,1Xs:M[nlH+*whM(*ff4;&TAFM%Ubd&-Y=F(x(4Q/Hw;pc"));
   U_ASSERT( n == 8612 )
   n = y.findSorted(U_STRING_FROM_CONSTANT("ayyuf;I=FLC?)G4<^$gftBB+P/1A]`L,I0^>@Y*71d0SDH^ '2E 7"));
   U_ASSERT( n == 8873 )

   ofstream outf("vector1.sort");

   outf << y;
   */

   y.clear();

   UString yA = U_STRING_FROM_CONSTANT("\n\n# comment line\n\nriga_0\nriga_1\n\n");

   n = y.split(yA);
   U_ASSERT( n == 2 )
   tmp = y[0];
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("riga_0") )
   tmp = y[1];
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("riga_1") )

   UString y0 = U_STRING_FROM_CONSTANT("word \"word with space\"");

   n = y.split(y0);
   U_ASSERT( n == 2 )
   tmp = y[2];
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("word") )
   tmp = y[3];
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("word with space") )

   UString y1 = U_STRING_FROM_CONSTANT("  word \"word with space\"    ");

   n = y.split(y1);
   U_ASSERT( n == 2 )
   tmp = y[4];
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("word") )
   tmp = y[5];
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("word with space") )

   tmp = y.join(U_CONSTANT_TO_PARAM("//"));
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("riga_0//riga_1//word//word with space//word//word with space") )

   y.clear();
   res = y.empty();
   U_ASSERT( res == true )

   UString y2 = U_STRING_FROM_CONSTANT("$Version=\"1\";\n Part_Number=\"Riding_Rocket_0023\"; $Path=\"/acme/ammo\";\n Part_Number=\"Rocket_Launcher_0001\"; $Path=\"/acme\"");

   n = y.split(y2, "=;, \n");
   U_ASSERT( n == 10 )

   tmp = y[0];
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("$Version") )
   tmp = y[1];
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("\"1\"") )
   tmp = y[2];
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("Part_Number") )
   tmp = y[3];
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("\"Riding_Rocket_0023\"") )
   tmp = y[4];
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("$Path") )
   tmp = y[5];
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("\"/acme/ammo\"") )
   tmp = y[6];
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("Part_Number") )
   tmp = y[7];
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("\"Rocket_Launcher_0001\"") )
   tmp = y[8];
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("$Path") )
   tmp = y[9];
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("\"/acme\"") )

   y.UVector<void*>::sort(compareObj);

   cout << y;

   y.clear();
   res = y.empty();
   U_ASSERT( res == true )

   y.deallocate();
   y.allocate(5);
   y.init();

   // RING BUFFER

   n = y.put(U_STRING_FROM_CONSTANT("riga 0"));
   U_ASSERT( n ==  0 )
   n = y.put(U_STRING_FROM_CONSTANT("riga 1"));
   U_ASSERT( n ==  0 )
   n = y.put(U_STRING_FROM_CONSTANT("riga 2"));
   U_ASSERT( n ==  0 )
   n = y.put(U_STRING_FROM_CONSTANT("riga 3"));
   U_ASSERT( n ==  0 )
   n = y.put(U_STRING_FROM_CONSTANT("riga 4"));
   U_ASSERT( n ==  0 )
   n = y.put(U_STRING_FROM_CONSTANT("riga 5"));
   U_ASSERT( n == unsigned(-1) )
   n = y.put(U_STRING_FROM_CONSTANT("riga 6"));
   U_ASSERT( n == unsigned(-1) )
   n = y.put(U_STRING_FROM_CONSTANT("riga 7"));
   U_ASSERT( n == unsigned(-1) )

   y.sort();

   tmp = y.get();
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("riga 3") )
   tmp = y.get();
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("riga 4") )
   tmp = y.get();
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("riga 5") )
   tmp = y.get();
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("riga 6") )
   tmp = y.get();
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("riga 7") )
   tmp = y.get();
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("") )

   y.init();

   n = y.put(U_STRING_FROM_CONSTANT("riga 0"));
   U_ASSERT( n ==  0 )
   n = y.put(U_STRING_FROM_CONSTANT("riga 1"));
   U_ASSERT( n ==  0 )
   n = y.put(U_STRING_FROM_CONSTANT("riga 2"));
   U_ASSERT( n ==  0 )
   n = y.put(U_STRING_FROM_CONSTANT("riga 3"));
   U_ASSERT( n ==  0 )
   n = y.put(U_STRING_FROM_CONSTANT("riga 4"));
   U_ASSERT( n ==  0 )
   n = y.put(U_STRING_FROM_CONSTANT("riga 5"));
   U_ASSERT( n == unsigned(-1) )

   y.resize(10);

   tmp = y.get();
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("riga 5") )

   n = y.put(U_STRING_FROM_CONSTANT("riga 6"));
   U_ASSERT( n ==  0 )
   n = y.put(U_STRING_FROM_CONSTANT("riga 7"));
   U_ASSERT( n ==  0 )

   tmp = y.get();
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("riga 1") )
   tmp = y.get();
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("riga 2") )
   tmp = y.get();
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("riga 3") )
   tmp = y.get();
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("riga 4") )
   tmp = y.get();
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("riga 6") )
   tmp = y.get();
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("riga 7") )

   res = y.empty();
   U_ASSERT( res == true )

   // BINARY HEAP

   y.bh_put(U_STRING_FROM_CONSTANT("riga 01"));
   tmp = y.bh_min();
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("riga 01") )

   y.bh_put(U_STRING_FROM_CONSTANT("riga 02"));
   tmp = y.bh_min();
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("riga 01") )

   y.bh_put(U_STRING_FROM_CONSTANT("riga 03"));
   tmp = y.bh_min();
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("riga 01") )

   y.bh_put(U_STRING_FROM_CONSTANT("riga 04"));
   tmp = y.bh_min();
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("riga 01") )

   y.bh_put(U_STRING_FROM_CONSTANT("riga 05"));
   tmp = y.bh_min();
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("riga 01") )

   y.bh_put(U_STRING_FROM_CONSTANT("riga 06"));
   tmp = y.bh_min();
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("riga 01") )

   y.bh_put(U_STRING_FROM_CONSTANT("riga 07"));
   tmp = y.bh_min();
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("riga 01") )

   y.bh_put(U_STRING_FROM_CONSTANT("riga 08"));
   tmp = y.bh_min();
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("riga 01") )

   y.bh_put(U_STRING_FROM_CONSTANT("riga 09"));
   tmp = y.bh_min();
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("riga 01") )

   y.bh_put(U_STRING_FROM_CONSTANT("riga 10"));
   tmp = y.bh_min();
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("riga 01") )

   y.bh_put(U_STRING_FROM_CONSTANT("riga 11"));
   tmp = y.bh_min();
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("riga 01") )

   tmp = y.bh_get();
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("riga 01") )
   tmp = y.bh_get();
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("riga 02") )
   tmp = y.bh_get();
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("riga 03") )
   tmp = y.bh_get();
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("riga 04") )
   tmp = y.bh_get();
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("riga 05") )
   tmp = y.bh_get();
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("riga 06") )
   tmp = y.bh_get();
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("riga 07") )
   tmp = y.bh_get();
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("riga 08") )
   tmp = y.bh_get();
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("riga 09") )
   tmp = y.bh_get();
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("riga 10") )
   tmp = y.bh_get();
   U_ASSERT( tmp == U_STRING_FROM_CONSTANT("riga 11") )

   res = y.empty();
   U_ASSERT( res == true )
}
