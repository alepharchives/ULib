// test_memerror.cpp

#include <ulib/debug/trace.h>
#include <ulib/debug/common.h>
#include <ulib/debug/error_memory.h>

class UInt {
public:
   // Check for memory error
   U_MEMORY_TEST

   // COSTRUTTORI

   UInt()      { a = 0; }
   UInt(int i) { a = i; }

   // ASSEGNAZIONI

   UInt(const UInt& i) : a(i.a)   {           U_MEMORY_TEST_COPY(i) }
   UInt& operator=(const UInt& i) { a = i.a;  U_MEMORY_TEST_COPY(i) return *this; }

   // CONVERSIONI

   operator int() const
      {
      U_CHECK_MEMORY

      return a;
      }

   // OPERATORI

   bool operator< (const UInt& i) const { return (a <  i.a); }
   bool operator==(const UInt& i) const { return (a == i.a); }

protected:
   int a;
};

int U_EXPORT main(int argc, char* argv[])
{
   u_init_ulib(argv);

   U_TRACE(5, "main(%d,%p)", argc, argv)

   UInt a(10);

   U_CHECK_MEMORY_OBJECT(&a)

   UInt* b = new UInt(a);

   delete b;

   int c = *b;

   *(int*)&a = 0x00ff;

   U_RETURN(0);
}
