// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    dynamic.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_DYNAMIC_H
#define ULIB_DYNAMIC_H 1

#include <ulib/internal/common.h>

#ifndef __MINGW32__
#  ifdef HAVE_DLFCN_H
extern "C" {
#     include <dlfcn.h>
}
#  endif
typedef void* HINSTANCE;
#endif

/**
   @class UDynamic
   @short  Dynamic class file loader.

   This class is used to load object files. On elf based systems this is typically done with dlopen.
*/

class U_EXPORT UDynamic {
public:

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   // COSTRUTTORI

   UDynamic()
      {
      U_TRACE_REGISTER_OBJECT(0, UDynamic, "")

      err    = "none";
      addr   = 0;
      handle = 0;
      }

   ~UDynamic()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UDynamic)
      }

   /** Load a object file
       @param filename pathname of object file to load
   */

   bool load(const char* filename);

   /** Detach a DSO object from running memory
   */

   void close();

   /** Lookup a symbol in the loaded file
   */

   void* operator[](const char* sym);

   /** Retrieve error indicator associated with DSO failure.
   */

   const char* getError() const { return err; }

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   void* addr;
   const char* err;
   HINSTANCE handle;

private:
   UDynamic(const UDynamic&)            {}
   UDynamic& operator=(const UDynamic&) { return *this; }
};

#endif
