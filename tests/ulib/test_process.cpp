// test_process.cpp

#include <ulib/timeval.h>
#include <ulib/process.h>
#include <ulib/notifier.h>

int
U_EXPORT main (int argc, char* argv[], char* environ[])
{
   U_ULIB_INIT(argv);

   U_TRACE(5,"main(%d)",argc)

   UProcess x;

   if (x.fork())
      {
      if (x.parent())
         {
         x.wait();

         U_ASSERT( x.exitValue() == 1536 )
         U_ASSERT( U_STRNCMP(x.exitInfo(), "Signal SIGABRT (6, Abort") == 0 )
         }
      else
         {
         U_ABORT("abort program for testing...", 0);
         }
      }
}
