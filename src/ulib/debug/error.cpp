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

#include <stdlib.h>

#ifdef HAVE_EXECINFO_H
#  include <execinfo.h>

void UError::stackDump()
{
   U_INTERNAL_TRACE("UError::stackDump()", 0)

   void* ba[20];

   // Store up to SIZE return address of the current program state in
   // ARRAY and return the exact number of values stored.

   int n = backtrace(ba, sizeof(ba)/ sizeof(ba[0]));

   if (n != 0)
      {
      // Return names of functions from the backtrace list in ARRAY in a newly malloc()ed memory block.

      /*
      char** names = backtrace_symbols(ba, n);

      if (names)
         {
         printf("called from %s\n", names[0]);

         for (int i = 1; i < n; ++i)
            {
            printf("            %s\n", names[i]);
            }

         free(names);
         }
      */

      char name[128];

      (void) u_snprintf(name, 128, "stack.%N.%P", 0);

      int fd = open(name, O_CREAT | O_WRONLY | O_APPEND | O_BINARY, 0666);

      // This function is similar to backtrace_symbols() but it writes the result
      // immediately to a file and can therefore also be used in situations where
      // malloc() is not usable anymore.

      backtrace_symbols_fd(ba, n, fd);

      (void) close(fd);
      }
}

#endif
