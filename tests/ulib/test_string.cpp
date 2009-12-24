// test_string.cpp

#include <ulib/string.h>
#include <ulib/debug/crono.h>
#include <ulib/utility/escape.h>
#include <ulib/utility/string_ext.h>

#ifdef HAVE_STRSTREAM_H
#  include <strstream.h>
#else
#  include <ulib/replace/strstream.h>
#endif

#undef min
using std::min;
#include <fstream>
#include <iomanip>

#define TEST_MEMMOVE
#define TEST_ASSIGN
#define TEST_CAPACITY
#define TEST_COMPARE
#define TEST_CONSTRUCTORS
#define TEST_ACCESS
#define TEST_FIND
#define TEST_INSERT
#define TEST_REPLACE
#define TEST_NOT_MEMBER
#define TEST_OPERATIONS
#define TEST_SUBSTR
#define TEST_CLASS
#define TEST_STREAM

#ifdef TEST_MEMMOVE
// memmove requirements
static void test_memmove_01()
{
   U_TRACE(5, "test_memmove_01()")

   const UString str_01("zuma beach");
   const UString str_02("montara and ocean beach");

   char array1[] = {'z', 'u', 'm', 'a', ' ', 'b', 'e', 'a', 'c', 'h',  0};
   const char str_lit1[] = "montara and ocean beach";
   char array2[sizeof(str_lit1) + sizeof(array1) - 1] = { '\0', '\0' };

   U_ASSERT( str_lit1[0] == 'm' );

   char c1 = array2[0];
   char c2 = str_lit1[0];
   char c3 = array2[1];
   char c4 = str_lit1[1];

// memmove(array2, str_lit1, 0);

   U_ASSERT( array2[0] == c1 );
   U_ASSERT( str_lit1[0] == c2 );

   memmove(array2, str_lit1, 1);

   U_ASSERT( array2[0] == c2 );
   U_ASSERT( str_lit1[0] == c2 );
   U_ASSERT( array2[1] == c3 );
   U_ASSERT( str_lit1[1] == c4 );

   memmove(array2, str_lit1, 2);

   U_ASSERT( array2[0] == c2 );
   U_ASSERT( str_lit1[0] == c2 );
   U_ASSERT( array2[1] == c4 );
   U_ASSERT( str_lit1[1] == c4 );

   char* pc1 = array1 + 1;
   c1 = pc1[0];
   c2 = array1[0];
   U_ASSERT( c1 != c2 );

   char* pc2 = (char*)memmove(array1, pc1, 0);

   c3 = pc1[0];
   c4 = array1[0];
   U_ASSERT( c1 == c3 );
   U_ASSERT( c2 == c4 );
   U_ASSERT( pc2 == array1 );

   c1 = pc1[0];
   c2 = array1[0];
   char* pc3 = pc1;

   pc2 = (char*)memmove(array1, pc1, 10);

   c3 = pc1[0];
   c4 = array1[0];
   U_ASSERT( c1 != c3 ); // underlying char array changed.
   U_ASSERT( c4 != c3 );
   U_ASSERT( pc2 == array1 );
   U_ASSERT( pc3 == pc1 ); // but pointers o-tay
   c1 = *(str_01.data());
   c2 = array1[0];
   U_ASSERT( c1 != c2 );
}
#endif

static unsigned csz01, csz02;
static unsigned npos = unsigned(-1);

#ifdef TEST_ASSIGN
// UString assign
static void test_assign_01()
{
   U_TRACE(5, "test_assign_01()")

   const char str_lit01[] = "point bolivar, texas";

   const UString str01(str_lit01);
   const UString str02("corpus, ");
   const UString str03;
         UString str05;

   // UString& append(const UString&)
   str05 = str02;
   str05.append(str05);
   U_ASSERT( str05 == "corpus, corpus, " );
   str05.append(str01);
   U_ASSERT( str05 == "corpus, corpus, point bolivar, texas" );
   str05.append(str03);
   U_ASSERT( str05 == "corpus, corpus, point bolivar, texas" );
   UString str06;
   str06.append(str05);
   U_ASSERT( str06 == str05 );

   // UString& append(const UString&, unsigned pos, unsigned n)
   str05.erase();
   str06.erase();

   str05 = str02;
   str05.append(str01, 0, npos);
   U_ASSERT( str05 == "corpus, point bolivar, texas" );
   U_ASSERT( str05 != str02 );

   str06 = str02;
   str06.append(str01, 15, npos);
   U_ASSERT( str06 == "corpus, texas" );
   U_ASSERT( str02 != str06 );

   // UString& append(const char* s)
   str05.erase();
   str06.erase();
   str05.append("");
   U_ASSERT( str05 == str03 );

   str05.append(str_lit01);
   U_ASSERT( str05 == str01 );

   str06 = str02;
   str06.append("corpus, ");
   U_ASSERT( str06 == "corpus, corpus, " );

   // UString& append(const char* s, unsigned n)
   str05.erase();
   str06.erase();
   str05.append("", 0);
   U_ASSERT( str05.size() == 0 );
   U_ASSERT( str05 == str03 );

   str05.append(str_lit01, sizeof(str_lit01) - 1);
   U_ASSERT( str05 == str01 );

   str06 = str02;
   str06.append("corpus, ", 6);
   U_ASSERT( str06 == "corpus, corpus" );

   str06 = str02;
   str06.append("corpus, ", 12);
   U_ASSERT( str06 != "corpus, corpus, " );

   // UString& append(unsigned n, char c)
   str05.erase();
   str06.erase();
   str05.append(0, 'a');
   U_ASSERT( str05 == str03 );
   str06.append(8, '.');
   U_ASSERT( str06 == "........" );

   str05.erase();
   str06.erase();
   str05.append(str03);
   U_ASSERT( str05 == str03 );

   str06 = str02;
   str06.append(str01.data(), str01.find('r')); 
   U_ASSERT( str06 == "corpus, point boliva" );
   U_ASSERT( str06 != str01 );
   U_ASSERT( str06 != str02 );

   str05 = str01;
   str05.append(str05.data(), str05.find('r')); 
   U_ASSERT( str05 ==  "point bolivar, texaspoint boliva" );
   U_ASSERT( str05 != str01 );
}
#endif

#ifdef TEST_CAPACITY
// UString capacity
static void test_capacity_01()
{
   U_TRACE(5, "test_capacity_01()")

   // 1 POD types : resize, capacity, reserve
   UString str01;

   unsigned sz01 = str01.capacity();
   str01.reserve(100);
   unsigned sz02 = str01.capacity();
   U_ASSERT( sz02 >= sz01 );
   U_ASSERT( sz02 >= 100 );
   str01.reserve(1);
   sz01 = str01.capacity();
   U_ASSERT( sz01 > 0 );

   sz01 = str01.size() + 5;
   str01.resize(sz01);
   sz02 = str01.size();
   U_ASSERT( sz01 == sz02 );

   sz01 = str01.size() - 5;
   str01.resize(sz01);
   sz02 = str01.size();
   U_ASSERT( sz01 == sz02 );

   UString str05(30, 'q');
   UString str06 = str05;
   str05 = str06 + str05;
   U_ASSERT( str05.capacity() >= str05.size() );
   U_ASSERT( str06.capacity() >= str06.size() );

   UString str02;
   unsigned sz03;
   unsigned sz04;

   sz03 = str02.capacity();
   str02.reserve(100);
   sz04 = str02.capacity();
   U_ASSERT( sz04 >= sz03 );
   U_ASSERT( sz04 >= 100 );
   str02.reserve(1);
   sz03 = str02.capacity();
   U_ASSERT( sz03 > 0 );

   sz03 = str02.size() + 5;
   str02.resize(sz03);
   sz04 = str02.size();
   U_ASSERT( sz03 == sz04 );

   sz03 = str02.size() - 5;
   str02.resize(sz03);
   sz04 = str02.size();
   U_ASSERT( sz03 == sz04 );

   char inst_obj = '\0';
   UString str07(30, inst_obj);
   UString str08 = str07;
   str07 = str08 + str07;
   U_ASSERT( str07.capacity() >= str07.size() );
   U_ASSERT( str08.capacity() >= str08.size() );

   // 3 POD types: size, length, max_size, clear(), empty()
   bool b01;
   UString str011;
   b01 = str01.empty();  
   U_ASSERT( b01 == true );
   sz01 = str01.size();
   sz02 = str01.length();
   U_ASSERT( sz01 == sz02 );
   str01.c_str();
   sz01 = str01.size();
   sz02 = str01.length();
   U_ASSERT( sz01 == sz02 );

   sz01 = str01.length();
   str01.c_str();
   str011 = str01 +  "_addendum_";
   str01.c_str();
   sz02 = str01.length();    
   U_ASSERT( sz01 == sz02 );
   sz02 = str011.length();
   U_ASSERT( sz02 > sz01 );

   // trickster allocator (__USE_MALLOC, luke) issues involved with these:
   UString str3 = U_STRING_FROM_CONSTANT("8-chars_8-chars_");
   const char* p3 = str3.c_str();
   UString str4 = str3 + U_STRING_FROM_CONSTANT("7-chars");
   const char* p4 = str3.c_str();
   p3 = p4;

   sz01 = str01.size();
   sz02 = str01.max_size();  
   U_ASSERT( sz02 >= sz01 );

   sz01 = str01.size();
   str01.clear();  
   b01 = str01.empty(); 
   U_ASSERT( b01 == true );
   sz02 = str01.size();  
   U_ASSERT( sz01 >= sz02 );

   b01 = str02.empty();  
   U_ASSERT( b01 == true );
   sz03 = str02.size();
   sz04 = str02.length();
   U_ASSERT( sz03 == sz04 );
   str02.c_str();
   sz03 = str02.size();
   sz04 = str02.length();
   U_ASSERT( sz03 == sz04 );

   sz03 = str02.max_size();  
   U_ASSERT( sz03 >= sz04 );

   sz03 = str02.size();
   str02.clear();  
   b01 = str02.empty(); 
   U_ASSERT( b01 == true );
   sz04 = str02.size();  
   U_ASSERT( sz03 >= sz04 );
}

static void test_capacity_02()
{
   U_TRACE(5, "test_capacity_02()")

   UString str01 = U_STRING_FROM_CONSTANT("twelve chars");
   // str01 becomes shared
   UString str02 = str01;
   str01.reserve(1);
   U_ASSERT( str01.capacity() >= 12 );
}
#endif

#ifdef TEST_COMPARE
// UString compare

// NB compare should be thought of as a lexographical compare, ie how
// things would be sorted in a dictionary.

enum want_value {lt=0, z=1, gt=2};

static void test_value(int result, want_value expected)
{
   U_TRACE(5, "test_value(%d,%d)", result, (int)expected)

   bool pass = false;

   switch (expected)
      {
      case lt:
         if (result < 0) pass = true;
      break;

      case z:
         if (!result) pass = true;
      break;

      case gt:
         if (result > 0) pass = true;
      break;

      default:
         pass = false; //should not get here
      }

   U_ASSERT( pass == true )
}
 
static void test_compare_01()
{
   U_TRACE(5, "test_compare_01()")

   UString str_0("costa rica");
   UString str_1("costa marbella");
   UString str_2;

   //sanity check
   test_value(strcmp("costa marbella", "costa rica"), lt); 
   test_value(strcmp("costa rica", "costa rica"), z);
   test_value(strcmp(str_1.data(), str_0.data()), lt);
   test_value(strcmp(str_0.data(), str_1.data()), gt);
   test_value(strncmp(str_1.data(), str_0.data(), 6), z);
   test_value(strncmp(str_1.data(), str_0.data(), 14), lt);
   test_value(memcmp(str_1.data(), str_0.data(), 6), z);
   test_value(memcmp(str_1.data(), str_0.data(), 14), lt);
   test_value(memcmp("costa marbella", "costa rica", 14), lt);

   // int compare(const UString& str) const;
   test_value(str_0.compare(str_1), gt); // because r>m
   test_value(str_1.compare(str_0), lt); // because m<r
   str_2 = str_0;
   test_value(str_2.compare(str_0), z);
   str_2 = "cost";
   test_value(str_2.compare(str_0), lt);
   str_2 = "costa ricans";
   test_value(str_2.compare(str_0), gt);

   // int compare(unsigned pos1, unsigned n1, const UString& str) const;
   test_value(str_1.compare(0, 6, str_0), lt);
   str_2 = "cost";
   test_value(str_1.compare(0, 4, str_2), z);
   test_value(str_1.compare(0, 5, str_2), gt);

   // int compare(unsigned pos1, unsigned n1, const UString& str, 
   //     unsigned pos2, unsigned n2) const; 
   test_value(str_1.compare(0, 6, str_0, 0, 6), z);
   test_value(str_1.compare(0, 7, str_0, 0, 7), lt);
   test_value(str_0.compare(0, 7, str_1, 0, 7), gt);

   // int compare(const char* s) const;
   test_value(str_0.compare("costa marbella"), gt);
   test_value(str_1.compare("costa rica"), lt);
   str_2 = str_0;
   test_value(str_2.compare("costa rica"), z);
   test_value(str_2.compare("cost"), gt);       
   test_value(str_2.compare("costa ricans"), lt);     

   // int compare(unsigned pos, unsigned n1, const char* str,
   //             unsigned n2 = npos) const;
   test_value(str_1.compare(0, 6, U_STRING_FROM_CONSTANT("costa rica"), 0, 6), z); 
   test_value(str_1.compare(0, 7, U_STRING_FROM_CONSTANT("costa rica"), 0, 7), lt); 
   test_value(str_0.compare(0, 7, U_STRING_FROM_CONSTANT("costa marbella"), 0, 7), gt); 
}
#endif

#ifdef TEST_CONSTRUCTORS
// UString constructors
static void test_constructors_01()
{
   U_TRACE(5, "test_constructors_01()")

   const char str_lit01[] = "rodeo beach, marin";

   const UString str01(str_lit01);
   const UString str02("baker beach, san francisco");

   // UString(const UString&, unsigned pos = 0, unsigned n = npos)
   csz01 = str01.size();
   UString str03(str01, csz01);
   U_ASSERT( str03.size() == 0 );
   U_ASSERT( str03.size() <= str03.capacity() );

   // UString(const char* s)
   UString str04(str_lit01);
   U_ASSERT( str01 == str04 );

   // UString(unsigned n, char c)
   csz01 = str01.max_size() / 1024;

   UString str05(csz01 - 1, 'z');
   U_ASSERT( str05.size() != 0 );
   U_ASSERT( str05.size() <= str05.capacity() );

   UString str06(str01.data(), str01.size());
   U_ASSERT( str06 == str01 );
}

static void test_constructors_02()
{
   U_TRACE(5, "test_constructors_02()")

   UString s(10,0);
   U_ASSERT( s.size() == 10 );
}

static void test_constructors_03()
{
   U_TRACE(5, "test_constructors_03()")

   const char* with_nulls = "This contains \0 a zero byte.";

   // These are tests to see how UString handles data with NUL
   // bytes.  Obviously UString(char*) will halt at the first one, but
   // nothing else should.
   UString s1 (with_nulls, 28);
   U_ASSERT( s1.size() == 28 );
   UString s2 (s1);
   U_ASSERT( s2.size() == 28 );
}
#endif

#ifdef TEST_ACCESS
// UString element access
static void test_access_01()
{
   U_TRACE(5, "test_access_01()")

   const UString str01("tamarindo, costa rica");
         UString str02("14th street beach, capitola, california");

   // char operator[] (unsigned pos) const;
   csz01 = str01.size();
   char ref1 = str01[csz01 - 1];
   U_ASSERT( ref1 == 'a' );
   ref1 = str01[csz01 - 2];
   U_ASSERT( ref1 == 'c' );

   csz02 = str02.size();
   char ref2 = str02[csz02 - 1];
   U_ASSERT( ref2 == 'a' );
   ref2 = str02[1];
   U_ASSERT( ref2 == '4' );

   // char at(unsigned pos) const;
   ref1 = str01.at(csz01 - 1);
   U_ASSERT( ref1 == 'a' );

   ref2 = str02.at(csz02 - 1);
   U_ASSERT( ref2 == 'a' );
}
#endif

#ifdef TEST_FIND
// UString find
static void test_find_01()
{
   U_TRACE(5, "test_find_01()")

   const char str_lit01[] = "mave";

   const UString str01("mavericks, santa cruz");
         UString str02(str_lit01);
         UString str03("s, s");
         UString str04;

   // unsigned find(const UString&, unsigned pos = 0) const;
   csz01 = str01.find(str01);
   U_ASSERT( csz01 == 0 );
   csz01 = str01.find(str01, 4);
   U_ASSERT( csz01 == npos );
   csz01 = str01.find(str02, 0);
   U_ASSERT( csz01 == 0 );
   csz01 = str01.find(str02, 3);
   U_ASSERT( csz01 == npos );
   csz01 = str01.find(str03, 0);
   U_ASSERT( csz01 == 8 );
   csz01 = str01.find(str03, 3);
   U_ASSERT( csz01 == 8 );
   csz01 = str01.find(str03, 12);
   U_ASSERT( csz01 == npos );

   // An empty UString consists of no characters
   // therefore it should be found at every point in a UString,
   // except beyond the end
   /*
   csz01 = str01.find(str04, 0);
   U_ASSERT( csz01 == 0 );
   csz01 = str01.find(str04, 5);
   U_ASSERT( csz01 == 5 );
   csz01 = str01.find(str04, str01.size());
   U_ASSERT( csz01 == str01.size() ); 
   csz01 = str01.find(str04, str01.size()+1);
   U_ASSERT( csz01 == npos ); 
   */

   // unsigned find(const char* s, unsigned pos, unsigned n) const;
   csz01 = str01.find(str_lit01, 0, 3);
   U_ASSERT( csz01 == 0 );
// csz01 = str01.find(str_lit01.c_str(), 3, 0);
// U_ASSERT( csz01 == 3 );

   // unsigned find(const char* s, unsigned pos = 0) const;
   csz01 = str01.find(str_lit01);
   U_ASSERT( csz01 == 0 );
   csz01 = str01.find(str_lit01, 3);
   U_ASSERT( csz01 == npos );

   // unsigned find(char c, unsigned pos = 0) const;
   csz01 = str01.find('z');
   csz02 = str01.size() - 1;
   U_ASSERT( csz01 == csz02 );
   csz01 = str01.find('/');
   U_ASSERT( csz01 == npos );

   // unsigned find_first_of(const UString&, unsigned pos = 0) const;
   UString str05("xena rulez");
   csz01 = str01.find_first_of(str01);
   U_ASSERT( csz01 == 0 );
   csz01 = str01.find_first_of(str01, 4);
   U_ASSERT( csz01 == 4 );
   csz01 = str01.find_first_of(str02, 0);
   U_ASSERT( csz01 == 0 );
   csz01 = str01.find_first_of(str02, 3);
   U_ASSERT( csz01 == 3 );
   csz01 = str01.find_first_of(str03, 0);
   U_ASSERT( csz01 == 8 );
   csz01 = str01.find_first_of(str03, 3);
   U_ASSERT( csz01 == 8 );
   csz01 = str01.find_first_of(str03, 12);
   U_ASSERT( csz01 == 16 );
   csz01 = str01.find_first_of(str05, 0);
   U_ASSERT( csz01 == 1 );
   csz01 = str01.find_first_of(str05, 4);
   U_ASSERT( csz01 == 4 );

   // An empty UString consists of no characters
   // therefore it should be found at every point in a UString,
   // except beyond the end
   // However, str1.find_first_of(str2,pos) finds the first character in 
   // str1 (starting at pos) that exists in str2, which is none for empty str2
   /*
   csz01 = str01.find_first_of(str04, 0);
   U_ASSERT( csz01 == npos );
   csz01 = str01.find_first_of(str04, 5);
   U_ASSERT( csz01 == npos );
   */

   // unsigned find_first_of(const char* s, unsigned pos, unsigned n) const;
   csz01 = str01.find_first_of(str_lit01, 0, 3);
   U_ASSERT( csz01 == 0 );
   csz01 = str01.find_first_of(str_lit01, 3, 0);
   U_ASSERT( csz01 == npos );

   // unsigned find_first_of(const char* s, unsigned pos = 0) const;
   csz01 = str01.find_first_of(str_lit01);
   U_ASSERT( csz01 == 0 );
   csz01 = str01.find_first_of(str_lit01, 3);
   U_ASSERT( csz01 == 3 );

   // unsigned find_first_of(char c, unsigned pos = 0) const;
   csz01 = str01.find_first_of('z');
   csz02 = str01.size() - 1;
   U_ASSERT( csz01 == csz02 );

   // unsigned find_last_of(const UString& str, unsigned pos = 0) const;
   // unsigned find_last_of(const char* s, unsigned pos, unsigned n) const;
   // unsigned find_last_of(const char* s, unsigned pos = 0) const;
   // unsigned find_last_of(char c, unsigned pos = 0) const;

   unsigned pos;
   UString x("X");
   pos = x.find_last_not_of('X');
   U_ASSERT( pos == npos );
   pos = x.find_last_not_of("XYZ");
   U_ASSERT( pos == npos );

   UString y("a");
   pos = y.find_last_not_of('X');
   U_ASSERT( pos == 0 );
   pos = y.find_last_not_of('a');
   U_ASSERT( pos == npos );
   pos = y.find_last_not_of("XYZ");
   U_ASSERT( pos == 0 );
   pos = y.find_last_not_of("a");
   U_ASSERT( pos == npos );

   UString z("ab");
   pos = z.find_last_not_of('X');
   U_ASSERT( pos == 1 );
   pos = z.find_last_not_of("XYZ");
   U_ASSERT( pos == 1 );
   pos = z.find_last_not_of('b');
   U_ASSERT( pos == 0 );
   pos = z.find_last_not_of("Xb");
   U_ASSERT( pos == 0 );
   pos = z.find_last_not_of("Xa");
   U_ASSERT( pos == 1 );
   pos = z.find_last_of("ab");
   U_ASSERT( pos == 1 );
   pos = z.find_last_of("Xa");
   U_ASSERT( pos == 0 );
   pos = z.find_last_of("Xb");
   U_ASSERT( pos == 1 );
   pos = z.find_last_of("XYZ");
   U_ASSERT( pos == npos );
   pos = z.find_last_of('a');
   U_ASSERT( pos == 0 );
   pos = z.find_last_of('b');
   U_ASSERT( pos == 1 );
   pos = z.find_last_of('X');
   U_ASSERT( pos == npos );
}

// UString rfind
static void test_rfind_01()
{
   U_TRACE(5, "test_rfind_01()")

   const char str_lit01[] = "mave";

   const UString str01("mavericks, santa cruz");
         UString str02(str_lit01);
         UString str03("s, s");
         UString str04;

   // unsigned rfind(const UString&, unsigned pos = 0) const;
   csz01 = str01.rfind(str01);
   U_ASSERT( csz01 == 0 );
   csz01 = str01.rfind(str01, 4);
   U_ASSERT( csz01 == 0 );
   csz01 = str01.rfind(str02,3);
   U_ASSERT( csz01 == 0 );
   csz01 = str01.rfind(str02);
   U_ASSERT( csz01 == 0 );
   csz01 = str01.rfind(str03);
   U_ASSERT( csz01 == 8 );
   csz01 = str01.rfind(str03, 3);
   U_ASSERT( csz01 == npos );
   csz01 = str01.rfind(str03, 12);
   U_ASSERT( csz01 == 8 );

   // An empty UString consists of no characters
   // therefore it should be found at every point in a UString,
   // except beyond the end
   csz01 = str01.rfind(str04, 0);
   U_ASSERT( csz01 == 0 );
   csz01 = str01.rfind(str04, 5);
   U_ASSERT( csz01 == 5 );
   csz01 = str01.rfind(str04, str01.size());
   U_ASSERT( csz01 == str01.size() );
   csz01 = str01.rfind(str04, str01.size()+1);
   U_ASSERT( csz01 == str01.size() );

   // unsigned rfind(const char* s, unsigned pos, unsigned n) const;
   csz01 = str01.rfind(str_lit01, 0, 3);
   U_ASSERT( csz01 == 0 );
   csz01 = str01.rfind(str_lit01, 3, 0);
   U_ASSERT( csz01 == 3 );

   // unsigned rfind(const char* s, unsigned pos = 0) const;
   csz01 = str01.rfind(str_lit01);
   U_ASSERT( csz01 == 0 );
   csz01 = str01.rfind(str_lit01, 3);
   U_ASSERT( csz01 == 0 );

   // unsigned rfind(char c, unsigned pos = 0) const;
   csz01 = str01.rfind('z');
   csz02 = str01.size() - 1;
   U_ASSERT( csz01 == csz02 );
   csz01 = str01.rfind('/');
   U_ASSERT( csz01 == npos );
}
#endif

#ifdef TEST_INSERT
// UString insert
static void test_insert_01()
{
   U_TRACE(5, "test_insert_01()")

   const UString str01("rodeo beach, marin");
   const UString str02("baker beach, san francisco");
         UString str03;

   // UString& insert(unsigned p1, const UString& str, unsigned p2, unsigned n)
   // requires:
   //   1) p1 <= size()
   //   2) p2 <= str.size()
   //   3) rlen = min(n, str.size() - p2)
   // conditions:
   //   1) (out_of_range) if p1 > size() || p2 > str.size()
   //   2) (length_error) if size() >= npos - rlen
   // effects:
   // replaces *this with new UString of length size() + rlen such that
   // nstr[0]  to nstr[p1] == thisstr[0] to thisstr[p1]
   // nstr[p1 + 1] to nstr[p1 + rlen] == str[p2] to str[p2 + rlen]
   // nstr[p1 + 1 + rlen] to nstr[...] == thisstr[p1 + 1] to thisstr[...]  
   str03 = str01; 
   csz01 = str03.size();
   csz02 = str02.size();

   str03.insert(13, str02, 0, 12); 
   U_ASSERT( str03 == "rodeo beach, baker beach,marin" );

   str03 = str01; 
   csz01 = str03.size();
   csz02 = str02.size();
   str03.insert(0, str02, 0, 12); 
   U_ASSERT( str03 == "baker beach,rodeo beach, marin" );

   str03 = str01;
   csz01 = str03.size();
   csz02 = str02.size();
   str03.insert(csz01, str02, 0, csz02); 
   U_ASSERT( str03 == "rodeo beach, marinbaker beach, san francisco" );

   // UString& insert(unsigned p, const UString& str);
   // insert(p1, str, 0, npos)
   str03 = str01;
   csz01 = str03.size();
   csz02 = str02.size();
   str03.insert(csz01, str02); 
   U_ASSERT( str03 == "rodeo beach, marinbaker beach, san francisco" );

   str03 = str01;
   csz01 = str03.size();
   csz02 = str02.size();
   str03.insert(0, str02); 
   U_ASSERT( str03 == "baker beach, san franciscorodeo beach, marin" );

   // UString& insert(unsigned p, const char* s, unsigned n);
   // insert(p1, UString(s,n))
   str03 = str02;
   csz01 = str03.size();
   str03.insert(0, "-break at the bridge", 20); 
   U_ASSERT( str03 == "-break at the bridgebaker beach, san francisco" );

   // UString& insert(unsigned p, const char* s);
   // insert(p1, UString(s))
   str03 = str02;
   str03.insert(0, "-break at the bridge"); 
   U_ASSERT( str03 == "-break at the bridgebaker beach, san francisco" );

   // UString& insert(unsigned p, unsigned n, char c)
   // insert(p1, UString(n,c))
   str03 = str02;
   csz01 = str03.size();
   str03.insert(csz01, 5, 'z');
   U_ASSERT( str03 == "baker beach, san franciscozzzzz" );

   // iterator insert(unsigned p, unsigned n, char c)
   // inserts n copies of c before the character referred to by p
   str03 = str02; 
   str03.insert(0, 1, 'u'); 
   U_ASSERT( str03 == "ubaker beach, san francisco" );

   str03 = str02; 
   str03.insert(0, 5, 'u'); 
   U_ASSERT( str03 == "uuuuubaker beach, san francisco" );

   str03 = str02; 
   csz01 = str03.size();
   str03.insert(0, str01);
   U_ASSERT( str03 == "rodeo beach, marinbaker beach, san francisco" );

   str03 = str02;
   csz01 = str03.size();
   str03.insert(str03.size(), str01);
   U_ASSERT( str03 == "baker beach, san franciscorodeo beach, marin" );
}
#endif

#ifdef TEST_REPLACE
// UString replace
static void test_replace_01()
{
   U_TRACE(5, "test_replace_01()")

   const char str_lit01[] = "ventura, california";

   const UString str01(str_lit01);
         UString str02("del mar, california");
         UString str03(" and ");
         UString str05;

   // UString& replace(unsigned pos,  unsigned n,  const UString& UString)
   // UString& replace(unsigned pos1, unsigned n1, const UString& UString,
   //                  unsigned pos2, unsigned n2)
   // UString& replace(unsigned pos,  unsigned n1, const char* s, unsigned n2)
   // UString& replace(unsigned pos,  unsigned n1, const char* s)
   // UString& replace(unsigned pos,  unsigned n1, unsigned n2, char c)

   UString X = U_STRING_FROM_CONSTANT("Hello");
   UString x = X;

   char ch = x[0];
   U_ASSERT( ch == 'H' );

   UString z = x.substr(2U, 3U);
   U_ASSERT( z == U_STRING_FROM_CONSTANT("llo") );

   x.replace(2, 2, "r");
   U_ASSERT( x == U_STRING_FROM_CONSTANT("Hero") );

   x = X;
   x.replace(0, 1, "j");
   U_ASSERT( x == U_STRING_FROM_CONSTANT("jello") );

   const char ar[] = { 'H', 'e', 'l', 'l', 'o', '\0' };
   x.replace(x.find('l'), 2, ar, (sizeof(ar) / sizeof(ar[0])) - 1);
   U_ASSERT( x == U_STRING_FROM_CONSTANT("jeHelloo") );
}
#endif

#ifdef TEST_NOT_MEMBER
// UString not_member functions
static void test_not_member_01()
{
   U_TRACE(5, "test_not_member_01()")

   UString str_0("costa rica");
   UString str_1("costa marbella");
   UString str_2("cost");
   UString str_3("costa ricans");
   UString str_4;

   str_4 = str_0;
   //comparisons between UString objects
   U_ASSERT( !(str_0 == str_1) );
   U_ASSERT( !(str_0 == str_2) );
   U_ASSERT( !(str_0 == str_3) );
   U_ASSERT( !(str_1 == str_0) );
   U_ASSERT( !(str_2 == str_0) );
   U_ASSERT( !(str_3 == str_0) );
   U_ASSERT( str_4 == str_0 );
   U_ASSERT( str_0 == str_4 );

   U_ASSERT( str_0 != str_1 );
   U_ASSERT( str_0 != str_2 );
   U_ASSERT( str_0 != str_3 );
   U_ASSERT( str_1 != str_0 );
   U_ASSERT( str_2 != str_0 );
   U_ASSERT( str_3 != str_0 );
   U_ASSERT( !(str_0 != str_4) );
   U_ASSERT( !(str_4 != str_0) );

   U_ASSERT( str_0 > str_1 ); //true cuz r>m
   U_ASSERT( str_0 > str_2 );
   U_ASSERT( !(str_0 > str_3) );
   U_ASSERT( !(str_1 > str_0) ); //false cuz m<r
   U_ASSERT( !(str_2 > str_0) );
   U_ASSERT( str_3 > str_0 );
   U_ASSERT( !(str_0 > str_4) );
   U_ASSERT( !(str_4 > str_0) );

   U_ASSERT( !(str_0 < str_1) ); //false cuz r>m
   U_ASSERT( !(str_0 < str_2) );
   U_ASSERT( str_0 < str_3 );
   U_ASSERT( str_1 < str_0 ); //true cuz m<r
   U_ASSERT( str_2 < str_0 );
   U_ASSERT( !(str_3 < str_0) );
   U_ASSERT( !(str_0 < str_4) );
   U_ASSERT( !(str_4 < str_0) );

   U_ASSERT( str_0 >= str_1 ); //true cuz r>m
   U_ASSERT( str_0 >= str_2 );
   U_ASSERT( !(str_0 >= str_3) );
   U_ASSERT( !(str_1 >= str_0) );//false cuz m<r
   U_ASSERT( !(str_2 >= str_0) );
   U_ASSERT( str_3 >= str_0 );
   U_ASSERT( str_0 >= str_4 );
   U_ASSERT( str_4 >= str_0 );

   U_ASSERT( !(str_0 <= str_1) );//false cuz r>m
   U_ASSERT( !(str_0 <= str_2) );
   U_ASSERT( str_0 <= str_3 );
   U_ASSERT( str_1 <= str_0 );//true cuz m<r
   U_ASSERT( str_2 <= str_0 );
   U_ASSERT( !(str_3 <= str_0) );
   U_ASSERT( str_0 <= str_4 );
   U_ASSERT( str_4 <= str_0 );

   //comparisons between UString object and UString literal
   U_ASSERT( !(str_0 == "costa marbella") );
   U_ASSERT( !(str_0 == "cost") );
   U_ASSERT( !(str_0 == "costa ricans") );
   U_ASSERT( !("costa marbella" == str_0) );
   U_ASSERT( !("cost" == str_0) );
   U_ASSERT( !("costa ricans" == str_0) );
   U_ASSERT( "costa rica" == str_0 );
   U_ASSERT( str_0 == "costa rica" );

   U_ASSERT( str_0 != "costa marbella" );
   U_ASSERT( str_0 != "cost" );
   U_ASSERT( str_0 != "costa ricans" );
   U_ASSERT( "costa marbella" != str_0 );
   U_ASSERT( "cost" != str_0 );
   U_ASSERT( "costa ricans" != str_0 );
   U_ASSERT( !("costa rica" != str_0) );
   U_ASSERT( !(str_0 != "costa rica") );

   U_ASSERT( str_0 > "costa marbella" ); //true cuz r>m
   U_ASSERT( str_0 > "cost" );
   U_ASSERT( !(str_0 > "costa ricans") );
   U_ASSERT( !("costa marbella" > str_0) );//false cuz m<r
   U_ASSERT( !("cost" > str_0) );
   U_ASSERT( "costa ricans" > str_0 );
   U_ASSERT( !("costa rica" > str_0) );
   U_ASSERT( !(str_0 > "costa rica") );

   U_ASSERT( !(str_0 < "costa marbella") );//false cuz r>m
   U_ASSERT( !(str_0 < "cost") );
   U_ASSERT( str_0 < "costa ricans" );
   U_ASSERT( "costa marbella" < str_0 );//true cuz m<r
   U_ASSERT( "cost" < str_0 );
   U_ASSERT( !("costa ricans" < str_0) );
   U_ASSERT( !("costa rica" < str_0) );
   U_ASSERT( !(str_0 < "costa rica") );

   U_ASSERT( str_0 >= "costa marbella" );//true cuz r>m
   U_ASSERT( str_0 >= "cost" );
   U_ASSERT( !(str_0 >= "costa ricans") );
   U_ASSERT( !("costa marbella" >= str_0) );//false cuz m<r
   U_ASSERT( !("cost" >= str_0) );
   U_ASSERT( "costa ricans" >= str_0 );
   U_ASSERT( "costa rica" >= str_0 );
   U_ASSERT( str_0 >= "costa rica" );

   U_ASSERT( !(str_0 <= "costa marbella") );//false cuz r>m
   U_ASSERT( !(str_0 <= "cost") );
   U_ASSERT( str_0 <= "costa ricans" );
   U_ASSERT( "costa marbella" <= str_0 );//true cuz m<r
   U_ASSERT( "cost" <= str_0 );
   U_ASSERT( !("costa ricans" <= str_0) );
   U_ASSERT( "costa rica" <= str_0 );
   U_ASSERT( str_0 <= "costa rica" );

   str_4 = str_0 + "ns";
   U_ASSERT( str_4 == str_3 );

   const UString str_5(" marbella");
   str_4 = "costa" + str_5;
   U_ASSERT( str_4 == str_1 );

   UString str_6("ns");
   str_4 = str_0 + str_6;
   U_ASSERT( str_4 == str_3 );

   str_4 = str_0 + 'n';
   str_4 = str_4 + 's';
   U_ASSERT( str_4 == str_3 );

   str_4 = 'a' + str_6;
   str_4 = 'c' + str_4;
   str_4 = 'i' + str_4;
   str_4 = 'r' + str_4;
   str_4 = ' ' + str_4;
   str_4 = 'a' + str_4;
   str_4 = 't' + str_4;
   str_4 = 's' + str_4;
   str_4 = 'o' + str_4;
   str_4 = 'c' + str_4;
   U_ASSERT( str_4 == str_3 );
}
#endif

#ifdef TEST_OPERATIONS
// UString operations
static void test_operations_01()
{
   U_TRACE(5, "test_operations_01()")

   UString str1;
   UString str2;

   // Should get this:
   // 1:8-chars_8-chars_
   // 2:8-chars_8-chars_
   str1 = UString("8-chars_") + "8-chars_";
   const char* p1 = str1.c_str();
   // printf("1:%s\n", str1.c_str());
   str2 = str1 + "7-chars";
   // printf("2:%s\n", str1.c_str()); //str1 is gone
   const char* p2 = str1.c_str();

   p1 = p2;
}
#endif

#ifdef TEST_SUBSTR
// UString substr
static void test_substr_01()
{
   U_TRACE(5, "test_substr_01()")

   const char str_lit01[] = "rockaway, pacifica";

   const UString str01(str_lit01);
         UString str02;

   // UString substr(unsigned pos = 0, unsigned n = npos) const;
   csz01 = str01.size();
   str02 = str01.substr(0U, 1U);
   U_ASSERT( str02 == "r" );
   str02 = str01.substr(10U);
   U_ASSERT( str02 == "pacifica" );

   str02 = str01.substr(csz01);
   U_ASSERT( str02.size() == 0 );
}
#endif

#ifdef TEST_CLASS
// class UString
// Do a quick sanity check on known problems with element access and
// ref-counted UStrings. These should all pass, regardless of the
// underlying UString implementation, of course.
static void test_class_01()
{
   U_TRACE(5, "test_class_01()")

         UString str01("montara beach, half moon bay");
   const UString str02("ocean beach, san francisco");
         UString str03;

   str03 = str01;

#ifdef U_STD_STRING
   typedef std::string::size_type csize_type;
   typedef std::string::iterator siterator;
   typedef std::string::reverse_iterator sriterator;
   siterator it1 =
#else
   char* it1 =
#endif
      str01._begin();

   *it1 = 'x';
   U_ASSERT( str01[0] == 'x' );
   U_ASSERT( str03[0] == 'm' );

   str03 = str01; 
   csz01 = str01.size();

#ifdef U_STD_STRING
   sriterator rit1 =
#else
   char* rit1 =
#endif
      str01._rbegin();

   *rit1 = 'z';
   U_ASSERT( str01[csz01 - 1] == 'z' );
   U_ASSERT( str03[csz01 - 1] == 'y' );

   str03 = str01;
   csz01 = str01.size();

/*
#ifdef U_STD_STRING
   std::string::reference r1 =
#else
   char& r1 =
#endif
      str01.at(csz01 - 2);

   U_ASSERT( str03 == str01 );
   r1 = 'd';
   U_ASSERT( str01[csz01 - 2] == 'd' );
   U_ASSERT( str03[csz01 - 2] == 'a' );

   str03 = str01; 
   csz01 = str01.size();

#ifdef U_STD_STRING
   std::string::reference r2 =
#else
   char& r2 =
#endif
      str01[csz01 - 3];

   U_ASSERT( str03 == str01 );
   r2 = 'w';
   U_ASSERT( str01[csz01 - 3] == 'w' );
   U_ASSERT( str03[csz01 - 3] == 'b' );
*/
   str03 = str01;
   csz02 = str01.size();
   it1 = str01._end();
   U_ASSERT( str03 == str01 );
   --it1;
   *it1 = 'q'; 
   U_ASSERT( str01[csz02 - 1] == 'q' );
   U_ASSERT( str03[csz02 - 1] == 'z' );

   str03 = str01;
   rit1 = str01._rend();
   U_ASSERT( str03 == str01 );
   --rit1;
   *rit1 = 'p';
   U_ASSERT( str01[0] == 'p' );
   U_ASSERT( str03[0] == 'x' );
}

// Do another sanity check, this time for member functions that return
// iterators, namely insert and erase.
static void test_class_02()
{
   U_TRACE(5, "test_class_02()")

   const UString str01("its beach, santa cruz");

   UString str02 = str01;
   UString str05 = str02;

#ifdef U_STD_STRING
   std::string::iterator p = str02.insert(str02.begin(), ' ');
#else
   char* p = str02.insert(0, 1, ' ')._begin();
#endif

   UString str03 = str02;
   U_ASSERT( str03 == str02 );
   *p = '!';

#ifdef U_STD_STRING
   U_ASSERT( *str03.c_str() == ' ' );
#endif

// str03[0] = '@';
   U_ASSERT( str02[0] == '!' );
   U_ASSERT( *p == '!' );
   U_ASSERT( str02 != str05 );
// U_ASSERT( str02 != str03 );

   UString str10 = str01;

#ifdef U_STD_STRING
   std::string::iterator p2 = str10.insert(str10.begin(), 'a');
#else
   char* p2 = str10.insert(0, 1, 'a')._begin();
#endif

   UString str11 = str10;
   *p2 = 'e';

#ifdef U_STD_STRING
   U_ASSERT( str11 != str10 );
#endif

   UString str06 = str01;
   UString str07 = str06;

#ifdef U_STD_STRING
   p = str06.erase(str06.begin());
#else
   p = str06.erase()._begin();
#endif

   UString str08 = str06;
   U_ASSERT( str08 == str06 );
   *p = '!';

#ifdef U_STD_STRING
   U_ASSERT( *str08.c_str() == 't' );
#endif

// str08[0] = '@';
   U_ASSERT( str06[0] == '!' );
   U_ASSERT( *p == '!' );
   U_ASSERT( str06 != str07 );

/*
#ifdef U_STD_STRING
   U_ASSERT( str06 != str08 );
#endif
*/

   UString str12 = str01;

#ifdef U_STD_STRING
   p2 = str12.erase(str12.begin(), str12.begin() + str12.size() - 1);
#else
   p2 = str12.erase(0, str12.size() - 1)._begin();
#endif

   UString str13 = str12;
   *p2 = 'e';

#ifdef U_STD_STRING
   U_ASSERT( str12 != str13 );
#endif
}
#endif

#ifdef TEST_STREAM
// UString inserters and extractors
static void test_stream_01()
{
   U_TRACE(5, "test_stream_01()")

   const UString str01("sailing grand traverse bay\n"
                 "\t\t\t    from Elk Rapids to the point reminds me of miles");
   const UString str02("sailing");
   const UString str03("grand");
   const UString str04("traverse");
   const UString str05;
         UString str10;

   // istream& operator>>(istream&, UString&)
   istrstream istrs01(U_STRING_TO_PARAM(str01));
   istrs01 >> str10;
   U_ASSERT( str10 == str02 );

   int i01 = istrs01.peek(); //a-boo
   U_ASSERT( i01 == ' ' );

   istrs01 >> str10; 
   U_ASSERT( str10 == str03 ); 
   istrs01 >> str10; 
   U_ASSERT( str10 == str04 ); // sentry picks out the white spaces. . 

#if __GNUC__ >= 3
   std::istrstream istrs02(U_STRING_TO_PARAM(str05)); // empty
   istrs02 >> str10;
   // U_ASSERT( str10 == str04 );
#endif

   // istream& getline(istream&, UString&, char)
   // istream& getline(istream&, UString&)
#ifndef U_STD_STRING
#  define getline(is, str, c) str.getline(is, c)
#endif

   getline(istrs01, str10, '\n');
   U_ASSERT( !istrs01.fail() );
   U_ASSERT( !istrs01.eof() );
   U_ASSERT( istrs01.good() );
   U_ASSERT( str10 == " bay" );

   istrs01.clear();
   getline(istrs01, str10, '\t');
   U_ASSERT( !istrs01.fail() );
   U_ASSERT( !istrs01.eof() );
   U_ASSERT( istrs01.good() );
   U_ASSERT( str10 == str05 );

   istrs01.clear();
   getline(istrs01, str10, '\t');
   U_ASSERT( !istrs01.fail() );
   U_ASSERT( !istrs01.eof() );
   U_ASSERT( istrs01.good() );
   U_ASSERT( str10 == str05 );

   istrs01.clear();
   getline(istrs01, str10, '.');
   U_ASSERT( !istrs01.fail() );
   U_ASSERT( istrs01.eof() );
   U_ASSERT( !istrs01.good() );
   U_ASSERT( str10 == "\t    from Elk Rapids to the point reminds me of miles" );
#if __GNUC__ >= 3
   getline(istrs02, str10, '\n');
   U_ASSERT( istrs02.fail() );
   U_ASSERT( istrs02.eof() );
   U_ASSERT( str10 == "\t    from Elk Rapids to the point reminds me of miles" );
#endif

   // ostream& operator<<(ostream&, const UString&)
   char buffer[1024];
   std::ostrstream ostrs01(buffer, sizeof(buffer));
   ostrs01 << str01 << '\0';
   U_ASSERT( str01 == ostrs01.str() );
}

// testing UStringbuf::xsputn via stress testing with large UStrings
// based on a bug report libstdc++ 9
static void test_stream_04(unsigned size)
{
   U_TRACE(5, "test_stream_04(%d)", size)

   UString str(size, 's');
   unsigned expected_size = (2 * (size + sizeof(char)));
   char buffer[expected_size+1];
   std::ostrstream oss(buffer, sizeof(buffer));

   // sanity checks
   U_ASSERT( str.size() == size );
   U_ASSERT( oss.good() );

   // stress test
   oss << str << std::endl;
   U_ASSERT( oss.good() );

   oss << str << std::endl << '\0';
   U_ASSERT( oss.good() );

   U_ASSERT( str.size() == size );
   U_ASSERT( oss.good() );
   UString str_tmp = UString(oss.str());
   U_ASSERT( str_tmp.size() == expected_size );
}

// testing filebuf::xsputn via stress testing with large UStrings
// based on a bug report libstdc++ 9 mode == out
static void test_stream_05(unsigned size)
{
   U_TRACE(5, "test_stream_05(%d)", size)

   const char* filename = "inserters_extractors-1.txt";
   const char fillc = 'f';
   ofstream ofs(filename);
   UString str(size, fillc);

   // sanity checks
   U_ASSERT( str.size() == size );
   U_ASSERT( ofs.good() );

   // stress test
   ofs << str << std::endl;
   U_ASSERT( ofs.good() == true );

   ofs << str << std::endl;
   U_ASSERT( ofs.good() == true );

   U_ASSERT( str.size() == size );

   ofs.close();

   // sanity check on the written file
   ifstream ifs(filename);
   unsigned count = 0;
   char c;
   while (count <= (2 * size) + 4)
      {
      ifs >> c;
      if (ifs.good() && c == fillc)
         {
         ++count;
         c = '0';
         }
      else 
         break;
      }

   U_ASSERT( count == 2 * size );

   unlink(filename);
}

// istrstream extractor properly size buffer based on
// actual, not allocated contents (UString.size() vs. UString.capacity()).
static void test_stream_06()
{
   U_TRACE(5, "test_stream_06()")

   UString str01("@silent");
   unsigned i01 = str01.size();
// unsigned i02 = str01.capacity();
   str01.erase(0, 1);
   unsigned i03 = str01.size();
   unsigned i04 = str01.capacity();
   U_ASSERT( i01 - 1 == i03 );
// U_ASSERT( i02 >= i04 );

   std::istrstream is(U_STRING_TO_PARAM(str01));
   UString str02;
   is >> str02;
   unsigned i05 = str02.size();
   unsigned i06 = str02.capacity();
   U_ASSERT( i05 == i03 );
// U_ASSERT( i06 <= i04 );
}

// istream::operator>>(UString)
// sets failbit
// NB: this is a defect in the standard.
static void test_stream_07()
{
   U_TRACE(5, "test_stream_07()")

   const UString name("z6.cc");
   std::istrstream iss(U_STRING_TO_PARAM(name));
   int i = 0;
   UString s;
   while (iss >> s) ++i;

   U_ASSERT( i < 3 );
   U_ASSERT( static_cast<bool>(iss.rdstate() & std::ios::failbit) );
}

static void test_stream_08()
{
   U_TRACE(5, "test_stream_08()")

   istrstream istrm(U_CONSTANT_TO_PARAM("enero :2001"));
   int      year = 0;
   char     sep = 0;
   UString  month;

   istrm >> month >> sep >> year;
   U_ASSERT( month.size() == 5 );
   U_ASSERT( sep == ':' );
   U_ASSERT( year == 2001 );
}

static void test_stream_09()
{
   U_TRACE(5, "test_stream_09()")

   UString blanks( 3, '0');
   UString foo = U_STRING_FROM_CONSTANT("peace");
   foo += blanks;
   foo += "& love";

   char buffer[1024];
   std::ostrstream oss1(buffer, sizeof(buffer));
   oss1 << foo << '\0';
   U_ASSERT( foo == oss1.str() );

   std::ostrstream oss2(buffer, sizeof(buffer));
   oss2.width(20);
   oss2 << foo;
   oss2.put('\0');
   U_ASSERT( foo != oss2.str() );
   U_ASSERT( strlen(oss2.str()) == 20 );
}
#endif

int
U_EXPORT main (int argc, char* argv[])
{
   U_ULIB_INIT(argv);

   U_TRACE(5, "main(%d)", argc)

   int n = (argc > 1 ? atoi(argv[1]) : 1);

#ifdef TEST_SPLIT
   test_split();
#endif

   UCrono crono;
   crono.start();

   for (int i = 0; i < n; ++i)
      {
#ifdef TEST_MEMMOVE
      test_memmove_01();
#endif
#ifdef TEST_ASSIGN
      test_assign_01();
#endif
#ifdef TEST_CAPACITY
      test_capacity_01();
      test_capacity_02();
#endif
#ifdef TEST_COMPARE
      test_compare_01();
#endif
#ifdef TEST_CONSTRUCTORS
      test_constructors_01();
      test_constructors_02();
      test_constructors_03();
#endif
#ifdef TEST_ACCESS
      test_access_01();
#endif
#ifdef TEST_FIND
      test_find_01();
      test_rfind_01();
#endif
#ifdef TEST_INSERT
      test_insert_01();
#endif
#ifdef TEST_REPLACE
      test_replace_01();
#endif
#ifdef TEST_NOT_MEMBER
      test_not_member_01();
#endif
#ifdef TEST_OPERATIONS
      test_operations_01();
#endif
#ifdef TEST_SUBSTR
      test_substr_01();
#endif
#ifdef TEST_CLASS
      test_class_01();
      test_class_02();
#endif
#ifdef TEST_STREAM
      test_stream_01();
      test_stream_04(1);      // expected_size == 4
      test_stream_04(1000);   // expected_size == 2002
      test_stream_04(10000);  // expected_size == 20002
      test_stream_05(1); 
      test_stream_05(1000); 
      test_stream_05(10000);
      test_stream_06();
      test_stream_07();
      test_stream_08();
      test_stream_09();
#endif
      }

   UString x = U_STRING_FROM_CONSTANT("/mnt/mirror/home/stefano/spool/cross");

   UString y = UStringExt::basename(x);

   U_ASSERT( y == "cross" )

   // 123456789012345678901234567890
   // 1     3  4     6  7  8

   y.setBuffer(100);

   UEscape::decode(U_CONSTANT_TO_PARAM("1\\t\\t3\\t4\\t\\t6\\t7\\t8"), y);

   y = UStringExt::expandTab(y, 3);

   U_ASSERT( y == "1     3  4     6  7  8" )

   y = UStringExt::simplifyWhiteSpace(U_CONSTANT_TO_PARAM("  lots\t of\nwhite    space "));

   U_ASSERT( y == "lots of white space" )

   y = UStringExt::stripWhiteSpace(U_CONSTANT_TO_PARAM("   lots of white space  "));

   U_ASSERT( y == "lots of white space" )

   y = UStringExt::insertEscape(U_CONSTANT_TO_PARAM("name=\"pippo\" file=\"pluto\""));

   U_ASSERT( y == "name=\\\"pippo\\\" file=\\\"pluto\\\"" )

   y = UStringExt::removeEscape(U_STRING_TO_PARAM(y));

   U_ASSERT( y == "name=\"pippo\" file=\"pluto\"" )

   U_ASSERT( U_STRING_FROM_CONSTANT("\t \n\r").isWhiteSpace() == true )

   y = U_STRING_FROM_CONSTANT("Hello");

   y = UStringExt::substitute(y, U_CONSTANT_TO_PARAM("l"), U_CONSTANT_TO_PARAM("ll"));

   U_ASSERT( y == "Hellllo" )

   UString z = y;

   y = UStringExt::substitute(y, U_CONSTANT_TO_PARAM("l"), U_CONSTANT_TO_PARAM("################################"));

   U_ASSERT( y == "He################################################################################################################################o" )

   z = UStringExt::substitute(z, U_CONSTANT_TO_PARAM("H"), U_CONSTANT_TO_PARAM("############################################################################################################################"));

   U_ASSERT( z == "############################################################################################################################ellllo")

   y = U_STRING_FROM_CONSTANT("Hello\n\n");

   z = UStringExt::dos2unix(y, true);

   U_ASSERT( z == "Hello\r\n\r\n" )

   z = UStringExt::dos2unix(z, true);

   U_ASSERT( z == "Hello\r\n\r\n" )

   z = UStringExt::dos2unix(z);

   U_ASSERT( z == y )

   // TID=trustACCESS1;UID=utente1;SID=;TS=20031201174127;CF=codicefiscale1
   z = U_STRING_FROM_CONSTANT("U2FsdGVkX1/QsrBvmsVHx0rrX78ldh6IJu1+4GhKoJ9O5ETSbfSiDip1gszkZX7w5ah6vkYfRWI8271LcNKhUsZVehRoscudLO8uotQgeiiF1B46ITphGw==");

   U_ASSERT( z.isBase64() == true )

   // manage pathname

   z = U_STRING_FROM_CONSTANT("/dir/base.suffix");

   U_ASSERT( UStringExt::suffix(z)   == U_STRING_FROM_CONSTANT("suffix") )
   U_ASSERT( UStringExt::dirname(z)  == U_STRING_FROM_CONSTANT("/dir") )
   U_ASSERT( UStringExt::basename(z) == U_STRING_FROM_CONSTANT("base.suffix") )

   /* Sort two version numbers, comparing equivalently seperated strings of digits numerically
    *
    * Returns a positive number if (a > b)
    * Returns a negative number if (a < b)
    * Returns zero if (a == b)
    */

   U_ASSERT( UStringExt::compareversion(U_STRING_FROM_CONSTANT("2.34"), U_STRING_FROM_CONSTANT("2.34")) == 0 )

   U_ASSERT( UStringExt::compareversion(U_STRING_FROM_CONSTANT("2.001"), U_STRING_FROM_CONSTANT("2.1")) == 0 )
   U_ASSERT( UStringExt::compareversion(U_STRING_FROM_CONSTANT("2.1"), U_STRING_FROM_CONSTANT("2.001")) == 0 )

   U_ASSERT( UStringExt::compareversion(U_STRING_FROM_CONSTANT("1.0.0"), U_STRING_FROM_CONSTANT("2.0.0")) < 0 )
   U_ASSERT( UStringExt::compareversion(U_STRING_FROM_CONSTANT("2.0.0"), U_STRING_FROM_CONSTANT("1.0.0")) > 0 )

   U_ASSERT( UStringExt::compareversion(U_STRING_FROM_CONSTANT(".0.0"), U_STRING_FROM_CONSTANT("2.0.0")) < 0 )
   U_ASSERT( UStringExt::compareversion(U_STRING_FROM_CONSTANT("2.0.0"), U_STRING_FROM_CONSTANT(".0.0")) > 0 )

   U_ASSERT( UStringExt::compareversion(U_STRING_FROM_CONSTANT("alpha"), U_STRING_FROM_CONSTANT("beta")) < 0 )
   U_ASSERT( UStringExt::compareversion(U_STRING_FROM_CONSTANT("beta"), U_STRING_FROM_CONSTANT("alpha")) > 0 )

   U_ASSERT( UStringExt::compareversion(U_STRING_FROM_CONSTANT("1.0"), U_STRING_FROM_CONSTANT("1.0.0")) < 0 )
   U_ASSERT( UStringExt::compareversion(U_STRING_FROM_CONSTANT("1.0.0"), U_STRING_FROM_CONSTANT("1.0")) > 0 )

   U_ASSERT( UStringExt::compareversion(U_STRING_FROM_CONSTANT("2.456"), U_STRING_FROM_CONSTANT("2.1000")) < 0 )
   U_ASSERT( UStringExt::compareversion(U_STRING_FROM_CONSTANT("2.1000"), U_STRING_FROM_CONSTANT("2.456")) > 0 )

   U_ASSERT( UStringExt::compareversion(U_STRING_FROM_CONSTANT("2.1000"), U_STRING_FROM_CONSTANT("3.111")) < 0 )
   U_ASSERT( UStringExt::compareversion(U_STRING_FROM_CONSTANT("3.111"), U_STRING_FROM_CONSTANT("2.1000")) > 0 )

   // recursively expand variables if needed

#define ENV_1     \
   "HOME=$HOME\n" \
   "PATH=$PATH\n" \
   "BRUTTOBUCO=$BRUTTOBUCO\n" \
   "LOGNAME=$LOGNAME$\n"

   UStringExt::putenv("HOME",1000);

   U_DUMP_ATTRS(environ);

   UStringExt::putenv("PATH","da qualche parte");

   U_DUMP_ATTRS(environ);

   UStringExt::putenv("LOGNAME", U_STRING_FROM_CONSTANT("bucaiolo"));

   U_DUMP_ATTRS(environ);

   z = UStringExt::expandEnvVar(U_STRING_FROM_CONSTANT(ENV_1));

#define ENV_2    \
   "HOME=1000\n" \
   "PATH=da qualche parte\n" \
   "BRUTTOBUCO=$BRUTTOBUCO\n" \
   "LOGNAME=bucaiolo$\n"

   U_ASSERT( z == U_STRING_FROM_CONSTANT(ENV_2) )

#ifdef HAVE_LIBZ
#  define TEXT3 \
"Apr 12 10:03:22 www sm-mta[2031]: l3C834YF002031: from=<tddiaz@thai.com>, size=2426, class=0, nrcpts=1, msgid=<c69d01c77cd8$9471501e$6aa9eea9@thai.com>, proto=SMTP, daemon=MTA, relay=adsl-d36.87-197-150.t-com.sk [87.197.150.36]\n" \
"Apr 12 10:03:22 www sm-mta[2034]: l3C834YF002031: to=<marcodd@unirel.it>, delay=00:00:13, xdelay=00:00:00, mailer=cyrusv2, pri=122426, relay=localhost, dsn=5.1.1, stat=User unknown\n" \
"Apr 12 10:03:22 www sm-mta[2034]: l3C834YF002031: l3C83MYF002034: DSN: User unknown"

   z = U_STRING_FROM_CONSTANT(TEXT3);

   U_ASSERT( z == UStringExt::gunzip(UStringExt::deflate(z)) )
#endif

   y = U_STRING_FROM_CONSTANT("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789+/");
   z = U_STRING_FROM_CONSTANT("abcdefghijklmnopqrstuvwxyz0123456789+/");

   U_ASSERT( UStringExt::tolower(y) == z )
   U_ASSERT( UStringExt::toupper(z) == y )

   U_ASSERT( UStringExt::isEmailAddress( U_STRING_FROM_CONSTANT("69d01c77cd8$9471501e$6aa9eea9@thai.com"))  == true )
   U_ASSERT( UStringExt::isEmailAddress( U_STRING_FROM_CONSTANT("69d01c77cd8$9471501e@6aa9eea9@thai.com")) == false )

   z = UString((void*)"buffer occupato");
   y = z;

   z.snprintf("%s", "pippo pluto paperino");

   U_ASSERT( U_STRING_FROM_CONSTANT("pippo pluto paperino") == z )

   z.snprintf("%s", "pippo.,;'?!!!");
   z.avoidPunctuation();

   U_ASSERT( U_STRING_FROM_CONSTANT("pippo") == z )

   // NB: in UTF-8 the character � is encoded as two bytes C3 (hex) and BC (hex)

   const char* str  = "binary: \xC3\xBC\x08\x01\x0B";
   uint32_t str_len = strlen(str);

   U_INTERNAL_ASSERT( u_isText((const unsigned char*)str, str_len)   == false )
   U_INTERNAL_ASSERT( u_isUTF8((const unsigned char*)str, str_len)   == false )
   U_INTERNAL_ASSERT( u_isUTF16((const unsigned char*)str, str_len)  == false )
   U_INTERNAL_ASSERT( u_isBinary((const unsigned char*)str, str_len) == true )

   z = UString::fromUTF8((const unsigned char*)str, str_len);

   U_INTERNAL_ASSERT( z.isUTF8()   == false )
   U_INTERNAL_ASSERT( z.isUTF16()  == false )
   U_INTERNAL_ASSERT( z.isText()   == false )
   U_INTERNAL_ASSERT( z.isBinary() == true )

   str     = "The string \xC3\xBC@foo-bar"; // "The string �@foo-bar"
   str_len = strlen(str);

   U_INTERNAL_ASSERT( u_isText((const unsigned char*)str, str_len)   == true )
   U_INTERNAL_ASSERT( u_isUTF8((const unsigned char*)str, str_len)   == true )
   U_INTERNAL_ASSERT( u_isUTF16((const unsigned char*)str, str_len)  == false )
   U_INTERNAL_ASSERT( u_isBinary((const unsigned char*)str, str_len) == false )

   z = UString::fromUTF8((const unsigned char*)str, str_len);

   U_INTERNAL_ASSERT( z.isText()   == true )
   U_INTERNAL_ASSERT( z.isUTF8()   == false )
   U_INTERNAL_ASSERT( z.isUTF16()  == false )
   U_INTERNAL_ASSERT( z.isBinary() == false )

   U_INTERNAL_ASSERT( UString::toUTF8((const unsigned char*)z.data(), z.size()) == str )

   U_INTERNAL_ASSERT( U_STRING_FROM_CONSTANT("           \n\t\r").isWhiteSpace() )
   U_INTERNAL_ASSERT( U_STRING_FROM_CONSTANT("gXWUj7VekBdkycg3Z9kXuglV9plUl2cs4XkNLSDhe5VHRgE03e63VypMChCWDGI=").isBase64() )

   crono.stop();

   printf("Time Consumed for (%d) iteration = %ld ms\n", n, crono.getTimeElapsed());

   U_RETURN(0);
}
