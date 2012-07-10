// ============================================================================
//
// = LIBRARY
//    ulibdbg - c++ library
//
// = FILENAME
//    error.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/debug/error.h>

#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_EXECINFO_H
#  include <execinfo.h>
#ifndef __GXX_ABI_VERSION
#define __GXX_ABI_VERSION 100
#endif
#  include <cxxabi.h>
#endif

void UError::stackDump()
{
   U_INTERNAL_TRACE("UError::stackDump()")

#ifdef HAVE_EXECINFO_H
   char name[128];
   void* array[256];

   (void) memset(array, 0, sizeof(void*) * 256);

   /* use -rdynamic flag when compiling */

   int size                    = backtrace(array, 256);
   char** __restrict__ strings = backtrace_symbols(array, size);

   (void) u__snprintf(name, sizeof(name), "stack.%N.%P", 0);

   int fd = open(name, O_CREAT | O_WRONLY | O_APPEND | O_BINARY, 0666);

   // This function is similar to backtrace_symbols() but it writes the result
   // immediately to a file and can therefore also be used in situations where
   // malloc() is not usable anymore.

   backtrace_symbols_fd(array, size, fd);

#  if defined(LINUX) || defined(__LINUX__) || defined(__linux__)
   int i, status;
   char* __restrict__ realname;
   char* __restrict__ lastparen;
   char* __restrict__ firstparen;

   FILE* __restrict__ f = fdopen(fd, "a");

   (void) fwrite(U_CONSTANT_TO_PARAM("=== STACK TRACE ===\n"), 1, f);

   /* we start from 3 to avoid
    * ------------------------
    * UError::stackDump()
    * u_debug_at_exit()
    * u__printf()
    * ------------------------
    */

   for (i = 3; i < size; ++i)
      {
      /* extract the identifier from strings[i]. It's inside of parens. */

      firstparen = strchr(strings[i], '(');
      lastparen  = strchr(strings[i], '+');

      if (firstparen &&
          lastparen  &&
          firstparen < lastparen)
         {
         *lastparen = '\0';

         realname = abi::__cxa_demangle(firstparen + 1, 0, 0, &status);

         if (realname)
            {
            (void) fprintf(f, "%s\n", realname);

            free(realname);
            }
         }
      }

   (void) fclose(f);
#  endif
#endif
}
