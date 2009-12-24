// test_objectIO.cpp

#include <ulib/internal/common.h>
#include <ulib/internal/objectIO.h>

#include <ulib/debug/trace.h>
#include <ulib/debug/common.h>
#include <ulib/debug/error_memory.h>

class Prima {
public:
   U_MEMORY_TEST

   const char* a;

   Prima() { a = "io_sono_la_classe_Prima"; }

   bool operator==(const Prima& l) { return U_STREQ(a, l.a); }
};

std::istream& operator>>(std::istream& is, Prima& l)
{
   static char buffer1[128];

   is >> buffer1;

   l.a = buffer1;

   return is;
}

std::ostream& operator<<(std::ostream& os, const Prima& l) 
{
   // per simulare ricorsione in U_OBJECT_TO_TRACE()

   U_TRACE(5, "Prima::operator<<()")

// U_INTERNAL_DUMP("this=%O", U_OBJECT_TO_TRACE(l))
   U_INTERNAL_DUMP("this=%O", 0)

   os << l.a;

   return os;
}

class Seconda {
public:
   U_MEMORY_TEST

   const char* b;

   Seconda() { b = "io_sono_la_classe_Seconda"; }

   bool operator==(const Seconda& l) { return U_STREQ(b, l.b); }
};

std::istream& operator>>(std::istream& is, Seconda& l)
{
   static char buffer2[128];

   is >> buffer2;

   l.b = buffer2;

   return is;
}

std::ostream& operator<<(std::ostream& os, const Seconda& l)
{
   // per simulare ricorsione in U_OBJECT_TO_TRACE()

   U_TRACE(5+256, "Seconda::operator<<()")

   Prima a1;

// U_INTERNAL_DUMP("a1=%O", U_OBJECT_TO_TRACE(a1))
   U_INTERNAL_DUMP("a1=%O", 0)

   os << l.b;

   return os;
}

int U_EXPORT main(int argc, char** argv)
{
   U_ULIB_INIT(argv);

   U_TRACE(5, "main(%d)", argc)

   U_ALLOCA(Prima   a, a1);
   U_ALLOCA(Seconda b, b1);

   U_INTERNAL_DUMP("Prima=%O",            U_OBJECT_TO_TRACE(a))
   U_INTERNAL_DUMP("Seconda=%O",                                  U_OBJECT_TO_TRACE(b))
   U_INTERNAL_DUMP("Prima=%O Seconda=%O", U_OBJECT_TO_TRACE(a),   U_OBJECT_TO_TRACE(b))

   U_SET_LOCATION_INFO;

   char* a_str = UObject2String(a);

   U_SET_LOCATION_INFO;

   if (strcmp(a_str, "io_sono_la_classe_Prima"))
      {
      U_ERROR("Error on UObject2String()...", 0);
      }

   UString2Object(U_CONSTANT_TO_PARAM("io_sono_la_classe_Prima"), a1);

   if (!(a == a1))
      {
      U_ERROR("Error on UString2Object()...", 0);
      }

   char* b_str = UObject2String(b);

   if (strcmp(b_str,"io_sono_la_classe_Seconda"))
      {
      U_ERROR("Error on UObject2String()...", 0);
      }

   UString2Object(U_CONSTANT_TO_PARAM("io_sono_la_classe_Seconda"), b1);

   if (!(b == b1))
      {
      U_ERROR("Error on UString2Object()...", 0);
      }
}
