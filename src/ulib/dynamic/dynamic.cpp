// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    dynamic.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/dynamic/dynamic.h>

bool UDynamic::load(const char* filename)
{
   U_TRACE(0, "UDynamic::load(%S)", filename)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_EQUALS(handle, 0)

#if defined(__MINGW32__)
   handle = ::LoadLibrary(U_PATH_CONV(filename));
#else
   /* Perform lazy binding
    * ---------------------
    * Only resolve symbols as the code that references them is executed.
    * If the symbol is never referenced, then it is never resolved.
    * (Lazy binding is only performed for function references; references to variables are always immediately bound when the library is loaded)
    */

   handle = U_SYSCALL(dlopen, "%S,%d", filename, RTLD_LAZY); // RTLD_NOW
#endif

   if (handle == 0)
      {
#  if defined(__MINGW32__)
      err = "load failed";
#  else
      err = ::dlerror();
#  endif

      U_INTERNAL_DUMP("err = %.*S", 256, err)

      U_RETURN(false);
      }

#ifndef __MINGW32__
   (void) ::dlerror(); /* Clear any existing error */
#endif

   U_RETURN(true);
}

void* UDynamic::operator[](const char* _sym)
{
   U_TRACE(0, "UDynamic::operator[](%S)", _sym)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_POINTER(handle)

   if (sym != _sym)
      {
#  if defined(__MINGW32__)
      addr = (void*) ::GetProcAddress(handle, _sym);
#  else
      addr = U_SYSCALL(dlsym, "%p,%S", handle, _sym);
#  endif

      if (addr) sym = _sym;
      else
         {
#     if defined(__MINGW32__)
         err = "symbol missing";
#     else
         err = ::dlerror();
#     endif

         U_INTERNAL_DUMP("err = %.*S", 256, err)
         }
      }

   U_RETURN(addr);
}

void UDynamic::close()
{
   U_TRACE(0, "UDynamic::close()")

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_POINTER(handle)

#if defined(__MINGW32__)
   ::FreeLibrary(handle);
#else
   (void) U_SYSCALL(dlclose, "%p", handle);
#endif

   sym    = 0;
   err    = 0;
   addr   = 0;
   handle = 0;
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UDynamic::dump(bool reset) const
{
   *UObjectIO::os << "err           " << err        << '\n'
                  << "sym           " << (void*)sym << '\n'
                  << "addr          " << addr       << '\n'
                  << "handle        " << (void*)handle;

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
