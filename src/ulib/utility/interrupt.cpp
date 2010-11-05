// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    interrupt.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/utility/interrupt.h>

#include <errno.h>
#include <stdlib.h>

/*
const char* UInterrupt::ILL_errlist[] = {
   "ILL_ILLOPC",  "illegal opcode",          // ILL_ILLOPC 1
   "ILL_ILLOPN",  "illegal operand",         // ILL_ILLOPN 2
   "ILL_ILLADR",  "illegal addressing mode", // ILL_ILLADR 3
   "ILL_ILLTRP",  "illegal trap",            // ILL_ILLTRP 4
   "ILL_PRVOPC",  "privileged opcode",       // ILL_PRVOPC 5
   "ILL_PRVREG",  "privileged register",     // ILL_PRVREG 6
   "ILL_COPROC",  "coprocessor error",       // ILL_COPROC 7
   "ILL_BADSTK",  "internal stack error"     // ILL_BADSTK 8
};
const char* UInterrupt::FPE_errlist[] = {
   "FPE_INTOVF",  "integer overflow",                 // FPE_INTOVF 2
   "FPE_FLTDIV",  "floating point divide by zero",    // FPE_FLTDIV 3
   "FPE_FLTOVF",  "floating point overflow",          // FPE_FLTOVF 4
   "FPE_FLTUND",  "floating point underflow",         // FPE_FLTUND 5
   "FPE_FLTRES",  "floating point inexact result",    // FPE_FLTRES 6
   "FPE_FLTINV",  "floating point invalid operation", // FPE_FLTINV 7
   "FPE_FLTSUB",  "subscript out of range"            // FPE_FLTSUB 8
};
const char* UInterrupt::CLD_list[] = {
   "CLD_EXITED",     "child has exited",              // CLD_EXITED     1
   "CLD_KILLED",     "child was killed",              // CLD_KILLED     2
   "CLD_DUMPED",     "child terminated abnormally",   // CLD_DUMPED     3
   "CLD_TRAPPED",    "traced child has trapped",      // CLD_TRAPPED    4
   "CLD_STOPPED",    "child has stopped",             // CLD_STOPPED    5
   "CLD_CONTINUED",  "stopped child has continued",   // CLD_CONTINUED  6
};
const char* UInterrupt::POLL_list[] = {
   "POLL_IN", "data input available",           // POLL_IN  1
   "POLL_OUT", "output buffers available",      // POLL_OUT 2
   "POLL_MSG", "input message available",       // POLL_MSG 3
   "POLL_ERR", "i/o error",                     // POLL_ERR 4
   "POLL_PRI", "high priority input available", // POLL_PRI 5
   "POLL_HUP", "device disconnected"            // POLL_HUP 6
};
const char* UInterrupt::TRAP_list[] = {
   "TRAP_BRKPT", "process breakpoint", // TRAP_BRKPT 1
   "TRAP_TRACE", "process trace trap", // TRAP_TRACE 2
};
const char* UInterrupt::origin_list[] = {
   "SI_USER",     "kill(), sigsend() or raise()",  // SI_USER      0
   "SI_QUEUE",    "sigqueue()",                    // SI_QUEUE    -1
   "SI_TIMER",    "timer expired",                 // SI_TIMER    -2
   "SI_MESGQ",    "mesq state changed",            // SI_MESGQ    -3
   "SI_ASYNCIO",  "AIO completed",                 // SI_ASYNCIO  -4
   "SI_SIGIO",    "queued SIGIO",                  // SI_SIGIO    -5
   "SI_KERNEL",   "kernel"                         // SI_KERNEL 0x80
};
*/

const char* UInterrupt::SEGV_errlist[] = {
   "", "",
   "SEGV_MAPERR", "Address not mapped to object",           // SEGV_MAPERR 1
   "SEGV_ACCERR", "Invalid permissions for mapped object"   // SEGV_ACCERR 2
};

const char* UInterrupt::BUS_errlist[] = {
   "", "",
   "BUS_ADRALN", "Invalid address alignment",      // BUS_ADRALN 1
   "BUS_ADRERR", "Non-existant physical address",  // BUS_ADRERR 2
   "BUS_OBJERR", "Object specific hardware error"  // BUS_OBJERR 3
};

sigset_t*         UInterrupt::mask_interrupt;
sig_atomic_t      UInterrupt::flag_wait_for_signal;

bool              UInterrupt::syscall_restart;     // NB: notify to make certain system calls restartable across signals...
int               UInterrupt::event_signal_pending;
sig_atomic_t      UInterrupt::event_signal[NSIG];
sighandler_t      UInterrupt::handler_signal[NSIG];
struct sigaction  UInterrupt::act;
struct sigaction  UInterrupt::old[NSIG];

void UInterrupt::insert(int signo, sighandler_t handler)
{
   U_TRACE(1, "UInterrupt::insert(%d,%p)", signo, handler)

   U_INTERNAL_ASSERT_RANGE(1, signo, NSIG)

   handler_signal[signo] = handler;

   act.sa_handler = handlerEventSignal;

   (void) U_SYSCALL(sigaction, "%d,%p,%p", signo, &act, old + signo);
}

void UInterrupt::erase(int signo)
{
   U_TRACE(1, "UInterrupt::erase(%d)", signo)

   U_INTERNAL_ASSERT_RANGE(1, signo, NSIG)
   U_INTERNAL_ASSERT_POINTER(handler_signal[signo])

   handler_signal[signo] = 0;

   (void) U_SYSCALL(sigaction, "%d,%p,%p", signo, old + signo, 0);
}

RETSIGTYPE UInterrupt::handlerSignal(int signo)
{
   U_TRACE(0, "[SIGNAL] UInterrupt::handlerSignal(%d)", signo)

   flag_wait_for_signal = false;
}

// Send ourselves the signal: see http://www.cons.org/cracauer/sigint.html

void UInterrupt::sendOurselves(int signo)
{
   U_TRACE(0, "UInterrupt::sendOurselves(%d)", signo)

   setHandlerForSignal(signo, (sighandler_t)SIG_DFL);

   u_exit();

   (void) U_SYSCALL(kill, "%d,%d", u_pid, signo);
}

RETSIGTYPE UInterrupt::handlerInterrupt(int signo)
{
   U_TRACE(1, "UInterrupt::handlerInterrupt(%d)", signo)

   U_MESSAGE("program interrupt - %Y", signo);

   // U_EXIT(-1);

   UInterrupt::sendOurselves(signo);
}

RETSIGTYPE UInterrupt::handlerEventSignal(int signo)
{
   U_TRACE(0, "[SIGNAL] UInterrupt::handlerEventSignal(%d)", signo)

   ++event_signal[signo];

   event_signal_pending = (event_signal_pending ? NSIG : signo);
}

void UInterrupt::waitForSignal(int signo)
{
   U_TRACE(1, "UInterrupt::waitForSignal(%d)", signo)

   static sigset_t mask_wait_for_signal;

   flag_wait_for_signal = true;

   setHandlerForSignal(signo, (sighandler_t)handlerSignal);

   while (flag_wait_for_signal == true)
      {
      (void) U_SYSCALL(sigsuspend, "%p", &mask_wait_for_signal);
      }
}

void UInterrupt::callHandlerSignal()
{
   U_TRACE(0, "UInterrupt::callHandlerSignal()")

   int i;

   do {
      i = event_signal_pending;
          event_signal_pending = 0;

      if (i < NSIG)
         {
         U_INTERNAL_ASSERT_POINTER(handler_signal[i])

         handler_signal[i](event_signal[i]);

         event_signal[i] = 0;
         }
      else
         {
         for (i = 1; i < NSIG; ++i)
            {
            if (event_signal[i])
               {
               U_INTERNAL_ASSERT_POINTER(handler_signal[i])

               handler_signal[i](event_signal[i]);

               event_signal[i] = 0;
               }
            }
         }
      }
   while (event_signal_pending);
}

void UInterrupt::setMaskInterrupt(sigset_t* mask, int signo)
{
   U_TRACE(1, "UInterrupt::setMaskInterrupt(%p,%d)", mask, signo)

   if (mask)
      {
#  ifdef sigemptyset
      sigemptyset(mask);
#  else
      (void) U_SYSCALL(sigemptyset, "%p", mask);
#  endif

#  ifdef sigaddset
      sigaddset(mask, signo);
#  else
      (void) U_SYSCALL(sigaddset, "%p,%d", mask, signo);
#  endif
      }
   else
      {
      U_INTERNAL_ASSERT_EQUALS(mask_interrupt,0)

      mask_interrupt = new sigset_t;

#  ifdef sigemptyset
      sigemptyset(mask_interrupt);
#  else
      (void) U_SYSCALL(sigemptyset, "%p", mask_interrupt);
#  endif

#  ifdef sigaddset
      sigaddset(mask_interrupt, SIGUSR1); // 10
      sigaddset(mask_interrupt, SIGUSR2); // 12
      sigaddset(mask_interrupt, SIGALRM); // 14
      sigaddset(mask_interrupt, SIGCHLD); // 17
#  else
      (void) U_SYSCALL(sigaddset, "%p,%d", mask_interrupt, SIGUSR1); // 10
      (void) U_SYSCALL(sigaddset, "%p,%d", mask_interrupt, SIGUSR2); // 12
      (void) U_SYSCALL(sigaddset, "%p,%d", mask_interrupt, SIGALRM); // 14
      (void) U_SYSCALL(sigaddset, "%p,%d", mask_interrupt, SIGCHLD); // 17
#  endif
      }
}

void UInterrupt::init()
{
   U_TRACE(1, "UInterrupt::init()")

#ifdef DEBUG
#  ifdef HAVE_SIGINFO_T
   act.sa_flags     = SA_SIGINFO;
   act.sa_sigaction = handlerInterruptWithInfo;

   (void) U_SYSCALL(sigaction, "%d,%p,%p", SIGBUS,  &act, 0); // 7
   (void) U_SYSCALL(sigaction, "%d,%p,%p", SIGSEGV, &act, 0); // 11
   /*
   (void) U_SYSCALL(sigaction, "%d,%p,%p", SIGCHLD, &act, 0);
   (void) U_SYSCALL(sigaction, "%d,%p,%p", SIGILL,  &act, 0);
   (void) U_SYSCALL(sigaction, "%d,%p,%p", SIGFPE,  &act, 0);
   (void) U_SYSCALL(sigaction, "%d,%p,%p", SIGPOLL, &act, 0);
   (void) U_SYSCALL(sigaction, "%d,%p,%p", SIGTRAP, &act, 0);
   */
#  endif

   act.sa_flags   = 0;
   act.sa_handler = handlerInterrupt;

// (void) U_SYSCALL(sigaction, "%d,%p,%p", SIGHUP,  &act, 0);
   (void) U_SYSCALL(sigaction, "%d,%p,%p", SIGINT,  &act, 0); // 2
// (void) U_SYSCALL(sigaction, "%d,%p,%p", SIGQUIT, &act, 0);
   (void) U_SYSCALL(sigaction, "%d,%p,%p", SIGTERM, &act, 0); // 15
#  ifndef __MINGW32__
// (void) U_SYSCALL(sigaction, "%d,%p,%p", SIGPIPE, &act, 0); // 13
#  endif

#  ifndef HAVE_SIGINFO_T
   (void) U_SYSCALL(sigaction, "%d,%p,%p", SIGSEGV, &act, 0); // 11
#  ifndef __MINGW32__
   (void) U_SYSCALL(sigaction, "%d,%p,%p", SIGBUS,  &act, 0); // 7
#  endif
#  endif
#endif

#ifndef __MINGW32__
   sigset_t mask_sigpipe;

   UInterrupt::setMaskInterrupt(&mask_sigpipe, SIGPIPE);

   UInterrupt::disable(&mask_sigpipe, 0);
#endif

   // NB: notify to make certain system calls restartable across signals...

   syscall_restart = true;
}

bool UInterrupt::checkForEventSignalPending()
{
   U_TRACE(0, "UInterrupt::checkForEventSignalPending()")

   U_INTERNAL_DUMP("errno = %d event_signal_pending = %d syscall_restart = %b", errno, event_signal_pending, syscall_restart)

   if (event_signal_pending) callHandlerSignal();

   // NB: notify to make certain system calls restartable across signals...

   U_RETURN(syscall_restart && errno == EINTR); // EINTR(4) Interrupted system call
}

#ifdef HAVE_SIGINFO_T

RETSIGTYPE UInterrupt::handlerInterruptWithInfo(int signo, siginfo_t* info, void* context)
{
   if (u_recursion == true) ::abort(); // NB: maybe recursion occurs...

#ifdef DEBUG
   u_flag_test = 0;

   UTrace utr(0, "UInterrupt::handlerInterruptWithInfo(%d,%p,%p)", signo, info, context);
#endif

   // #if defined(__CYGWIN__)
   // union sigval {
   // int    sival_int; /* Integer signal value */
   // void  *sival_ptr; /* Pointer signal value */ }
   //
   // siginfo_t {
   // int          si_signo; /* Signal number */
   // int          si_code;  /* Cause of the signal */
   // union sigval si_value; /* Signal value */ }
   // #else
   // siginfo_t {
   //    int      si_signo;  /* Signal number */
   //    int      si_errno;  /* An errno value */
   //    int      si_code;   /* Signal code */
   //    pid_t    si_pid;    /* Sending process ID */
   //    uid_t    si_uid;    /* Real user ID of sending process */
   //    int      si_status; /* Exit value or signal */
   //    clock_t  si_utime;  /* User time consumed */
   //    clock_t  si_stime;  /* System time consumed */
   //    sigval_t si_value;  /* Signal value */
   //    int      si_int;    /* POSIX.1b signal */
   //    void*    si_ptr;    /* POSIX.1b signal */
   //    void*    si_addr;   /* Memory location which caused fault */
   //    int      si_band;   /* Band event */
   //    int      si_fd;     /* File descriptor */ }
   // #  endif

#  ifndef    SI_FROMKERNEL
#     define SI_FROMKERNEL(siptr) ((siptr)->si_code > 0)
#  endif

   if (SI_FROMKERNEL(info))
      {
      const char** errlist = (signo == SIGBUS ? BUS_errlist : SEGV_errlist);

      int index = info->si_code * 2;

#  if defined(HAVE_MEMBER_SI_ADDR)
      U_ABORT("program interrupt by the kernel - %Y%W\n"
              "----------------------------------------"
              "----------------------------------------------------\n"
              " pid: %W%P%W\n"
              " address: %W%p - %s (%d, %s)%W\n"
              "----------------------------------------"
              "----------------------------------------------------",
              signo, YELLOW,
              CYAN, YELLOW,
              CYAN, info->si_addr, errlist[index], info->si_code, errlist[index+1], YELLOW);
#  else
      U_ABORT("program interrupt by the kernel - %Y%W\n"
              "----------------------------------------"
              "----------------------------------------------------\n"
              " pid: %W%P%W\n"
              " desc: %W %s (%d, %s)%W\n"
              "----------------------------------------"
              "----------------------------------------------------",
              signo, YELLOW,
              CYAN, YELLOW,
              CYAN, errlist[index], info->si_code, errlist[index+1], YELLOW);
#  endif
      }

   U_ABORT("program interrupt by kill(), sigsend() or raise() - %Y", signo);
}

#endif
