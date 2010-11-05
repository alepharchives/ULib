// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    interrupt.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_INTERRUPT_H
#define ULIB_INTERRUPT_H 1

#include <ulib/internal/common.h>

struct U_EXPORT UInterrupt {

   static struct sigaction act;

   /*
   static const char* CLD_list[];
   static const char* POLL_list[];
   static const char* TRAP_list[];
   static const char* ILL_errlist[];
   static const char* FPE_errlist[];
   static const char* origin_list[];
   */
   static const char* BUS_errlist[];
   static const char* SEGV_errlist[];

   static void init();
   static void setMaskInterrupt(sigset_t* mask, int signo);

   static RETSIGTYPE handlerInterrupt(int signo);

#ifdef HAVE_SIGINFO_T
   static RETSIGTYPE handlerInterruptWithInfo(int signo, siginfo_t* info, void*); // __attribute__ ((noreturn));
#endif

   static sigset_t* mask_interrupt; // SIGALRM | SIGUSR[1|2] | SIGCHLD

   static bool disable(sigset_t* mask, sigset_t* mask_old)
      {
      U_TRACE(1, "UInterrupt::disable(%p,%p)", mask, mask_old)

      if (!mask)
         {
         if (!mask_interrupt) setMaskInterrupt(0, 0);

         mask = mask_interrupt;
         }

      bool result = (U_SYSCALL(sigprocmask, "%d,%p,%p", SIG_BLOCK, mask, mask_old) == 0);

      U_RETURN(result);
      }

   static bool enable(sigset_t* mask)
      {
      U_TRACE(1, "UInterrupt::enable(%p)", mask)

      if (!mask)
         {
         if (!mask_interrupt) setMaskInterrupt(0, 0);

         mask = mask_interrupt;
         }

      bool result = (U_SYSCALL(sigprocmask, "%d,%p,%p", SIG_BLOCK, mask, 0) == 0);

      U_RETURN(result);
      }

   static sig_atomic_t flag_wait_for_signal;

   static RETSIGTYPE handlerSignal(int signo);

   static void setHandlerForSignal(int signo, sighandler_t function)
      {
      U_TRACE(1, "UInterrupt::setHandlerForSignal(%d,%p)", signo, function)

      act.sa_handler = function;

      (void) U_SYSCALL(sigaction, "%d,%p,%p", signo, &act, 0);
      }

   static void waitForSignal(int signo);
   static void sendOurselves(int signo);
   static bool sendSignal(   int signo, pid_t pid)
      {
      U_TRACE(1, "UInterrupt::sendSignal(%d,%d)", signo, pid)

      bool ok = (U_SYSCALL(kill, "%d,%d", pid, signo) != -1);

      U_RETURN(ok);
      }

   // manage async signal

   static bool syscall_restart; // NB: notify to make certain system calls restartable across signals...
   static int event_signal_pending;
   static struct sigaction old[NSIG];
   static sig_atomic_t event_signal[NSIG];
   static sighandler_t handler_signal[NSIG];

   static void  erase(int signo);
   static void insert(int signo, sighandler_t handler);

   static RETSIGTYPE handlerEventSignal(int signo);

   static void callHandlerSignal();
   static bool checkForEventSignalPending();
};

#endif
