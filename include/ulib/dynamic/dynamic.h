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


#ifdef __MINGW32__
#  define U_LIB_SUFFIX  "dll"
#  define U_FMT_LIBPATH "%s/%.*s." U_LIB_SUFFIX
#else
#  define U_LIB_SUFFIX  "so"
#  define U_FMT_LIBPATH "%s/%.*s." U_LIB_SUFFIX
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
      U_TRACE(0, "UDynamic::UDynamic()")

      err    = "none";
      addr   = 0;
      handle = 0;
      }

   ~UDynamic()
      {
      U_TRACE(0, "UDynamic::~UDynamic()")
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

   static void* getAddressOfFunction(const char* name);

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   void* addr;
   const char* err;
   HINSTANCE handle;

   static UDynamic* pmain;

private:
   UDynamic(const UDynamic&)            {}
   UDynamic& operator=(const UDynamic&) { return *this; }
};

#endif
