// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    ruby.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_RUBY_H
#define ULIB_RUBY_H 1

#include <ulib/dynamic/dynamic.h>

class U_EXPORT URUBY : public UDynamic {
public:

   // COSTRUTTORI

   URUBY()
      {
      U_TRACE_REGISTER_OBJECT(0, URUBY, "", 0)

      result = 0;
      }

   ~URUBY()
      {
      U_TRACE_UNREGISTER_OBJECT(0, URUBY)
      }

   bool load(const char* libname = "libruby.so") { return UDynamic::load(libname); }

   // RUBY operations

   bool run(int argc, char** argv);

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   int result;

   void setError();

private:
   URUBY(const URUBY&)            {}
   URUBY& operator=(const URUBY&) { return *this; }
};

#endif
