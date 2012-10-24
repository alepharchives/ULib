// test_interrupt.cpp

#include <ulib/base/utility.h>
#include <ulib/utility/interrupt.h>

#ifdef __MINGW32__
#  include <process.h>
#else
#  include <sys/mman.h>
#endif

static RETSIGTYPE handlerForAlarm(int signo)
{
   U_TRACE(5,"handlerForAlarm(%d)", signo)

   UInterrupt::sendSignal(SIGUSR1, getpid());
}

int
U_EXPORT main (int argc, char* argv[])
{
   U_ULIB_INIT(argv);

   U_TRACE(5,"main(%d)",argc)

#ifndef DEBUG
   UInterrupt::init();
#endif
   UInterrupt::setHandlerForSignal(SIGALRM, (sighandler_t)handlerForAlarm);

   alarm(1);

   UInterrupt::waitForSignal(SIGUSR1);

   /*
   char* ptr = (char*)0x013;
   *ptr = '\0'; // SIGSEGV

   int fd = open("tmp",O_CREAT|O_RDWR,0666);
   write(fd,string_and_size("aaaaaaaaaaaaaaaaaaaaaaaa"));
   ptr = (char*) mmap(NULL, 24, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
   ftruncate(fd, 0);
   close(fd);
   ptr[20] = '\0'; // SIGBUS
   */

// putenv("EXEC_ON_EXIT=/utility/stack_extend.pl");

   char Buffer[] = "is this a violation?";

   U__MEMCPY(&Buffer[5], &Buffer[10], 10U); /*Possible Violation*/

   U_WARNING("%s", "test for SIGSEGV from user");

   UInterrupt::sendSignal(SIGSEGV, getpid());

   U_RETURN(0);
}
