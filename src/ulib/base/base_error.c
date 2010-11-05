/** ============================================================================
//
// = LIBRARY
//    ulibase - c library
//
// = FILENAME
//    base_error.c
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================ */

/*
#define DEBUG_DEBUG
*/

#include <ulib/base/error.h>

#include <errno.h>
#include <stdlib.h>

#ifdef HAVE_SYSEXITS_H
#  include <sysexits.h>
#else
#  include <ulib/base/replace/sysexits.h>
#endif

#define U_STR_ERROR "\n" \
" ----------------------------------- \n" \
"| *****  ****   ****    ***   ****  |\n" \
"| *      *   *  *   *  *   *  *   * |\n" \
"| *****  ****   ****   *   *  ****  |\n" \
"| *      *  *   *  *   *   *  *  *  |\n" \
"| *****  *   *  *   *   ***   *   * |\n" \
" ----------------------------------- \n"

void u_printError(void)
{
   U_INTERNAL_TRACE("u_printError()")

   if (u_is_tty) (void) write(STDERR_FILENO, U_CONSTANT_TO_PARAM(U_RED_STR));
                 (void) write(STDERR_FILENO, U_CONSTANT_TO_PARAM(U_STR_ERROR));
   if (u_is_tty) (void) write(STDERR_FILENO, U_CONSTANT_TO_PARAM(U_RESET_STR));
}

/*
Maps errno number to an error message string, the contents of which are implementation defined.
The returned string is only guaranteed to be valid only until the next call to function.
*/

const char* u_getSysError(uint32_t* restrict len)
{
   /*
   Translation table for errno values. See intro(2) in most UNIX systems Programmers Reference Manuals.
   Note that this table is generally only accessed when it is used at runtime to initialize errno name and
   message tables that are indexed by errno value.
   Not all of these errnos will exist on all systems. This table is the only thing that should have to be
   updated as new error numbers are introduced. It's sort of ugly, but at least its portable.
   */

   struct error_info
      {
      int                  value;   /* The numeric value from <errno.h> */
      const char* restrict name;    /* The equivalent symbolic value */
#  ifndef HAVE_STRERROR
      const char* restrict msg;     /* Short message about this value */
#  endif
      };

#ifdef HAVE_STRERROR
#   define ENTRY(value,name,msg) {value,name}
#else
#   define ENTRY(value,name,msg) {value,name,msg}
#endif

   static const struct error_info error_table[] = {
#if defined(EPERM)
   ENTRY(EPERM, "EPERM", "Not owner"),
#endif
#if defined(ENOENT)
   ENTRY(ENOENT, "ENOENT", "No such file or directory"),
#endif
#if defined(ESRCH)
   ENTRY(ESRCH, "ESRCH", "No such process"),
#endif
#if defined(EINTR)
   ENTRY(EINTR, "EINTR", "Interrupted system call"),
#endif
#if defined(EIO)
   ENTRY(EIO, "EIO", "I/O error"),
#endif
#if defined(ENXIO)
   ENTRY(ENXIO, "ENXIO", "No such device or address"),
#endif
#if defined(E2BIG)
   ENTRY(E2BIG, "E2BIG", "Argument list too long"),
#endif
#if defined(ENOEXEC)
   ENTRY(ENOEXEC, "ENOEXEC", "Exec format error"),
#endif
#if defined(EBADF)
   ENTRY(EBADF, "EBADF", "Bad file number"),
#endif
#if defined(ECHILD)
   ENTRY(ECHILD, "ECHILD", "No child processes"),
#endif
#if defined(EWOULDBLOCK)   /* Put before EAGAIN, sometimes aliased */
   ENTRY(EWOULDBLOCK, "EWOULDBLOCK", "Operation would block"),
#endif
#if defined(EAGAIN)
   ENTRY(EAGAIN, "EAGAIN", "No more processes"),
#endif
#if defined(ENOMEM)
   ENTRY(ENOMEM, "ENOMEM", "Not enough space"),
#endif
#if defined(EACCES)
   ENTRY(EACCES, "EACCES", "Permission denied"),
#endif
#if defined(EFAULT)
   ENTRY(EFAULT, "EFAULT", "Bad address"),
#endif
#if defined(ENOTBLK)
   ENTRY(ENOTBLK, "ENOTBLK", "Block device required"),
#endif
#if defined(EBUSY)
   ENTRY(EBUSY, "EBUSY", "Device busy"),
#endif
#if defined(EEXIST)
   ENTRY(EEXIST, "EEXIST", "File exists"),
#endif
#if defined(EXDEV)
   ENTRY(EXDEV, "EXDEV", "Cross-device link"),
#endif
#if defined(ENODEV)
   ENTRY(ENODEV, "ENODEV", "No such device"),
#endif
#if defined(ENOTDIR)
   ENTRY(ENOTDIR, "ENOTDIR", "Not a directory"),
#endif
#if defined(EISDIR)
   ENTRY(EISDIR, "EISDIR", "Is a directory"),
#endif
#if defined(EINVAL)
   ENTRY(EINVAL, "EINVAL", "Invalid argument"),
#endif
#if defined(ENFILE)
   ENTRY(ENFILE, "ENFILE", "File table overflow"),
#endif
#if defined(EMFILE)
   ENTRY(EMFILE, "EMFILE", "Too many open files"),
#endif
#if defined(ENOTTY)
   ENTRY(ENOTTY, "ENOTTY", "Not a typewriter"),
#endif
#if defined(ETXTBSY)
   ENTRY(ETXTBSY, "ETXTBSY", "Text file busy"),
#endif
#if defined(EFBIG)
   ENTRY(EFBIG, "EFBIG", "File too large"),
#endif
#if defined(ENOSPC)
   ENTRY(ENOSPC, "ENOSPC", "No space left on device"),
#endif
#if defined(ESPIPE)
   ENTRY(ESPIPE, "ESPIPE", "Illegal seek"),
#endif
#if defined(EROFS)
   ENTRY(EROFS, "EROFS", "Read-only file system"),
#endif
#if defined(EMLINK)
   ENTRY(EMLINK, "EMLINK", "Too many links"),
#endif
#if defined(EPIPE)
   ENTRY(EPIPE, "EPIPE", "Broken pipe"),
#endif
#if defined(EDOM)
   ENTRY(EDOM, "EDOM", "Math argument out of domain of func"),
#endif
#if defined(ERANGE)
   ENTRY(ERANGE, "ERANGE", "Math result not representable"),
#endif
#if defined(ENOMSG)
   ENTRY(ENOMSG, "ENOMSG", "No message of desired type"),
#endif
#if defined(EIDRM)
   ENTRY(EIDRM, "EIDRM", "Identifier removed"),
#endif
#if defined(ECHRNG)
   ENTRY(ECHRNG, "ECHRNG", "Channel number out of range"),
#endif
#if defined(EL2NSYNC)
   ENTRY(EL2NSYNC, "EL2NSYNC", "Level 2 not synchronized"),
#endif
#if defined(EL3HLT)
   ENTRY(EL3HLT, "EL3HLT", "Level 3 halted"),
#endif
#if defined(EL3RST)
   ENTRY(EL3RST, "EL3RST", "Level 3 reset"),
#endif
#if defined(ELNRNG)
   ENTRY(ELNRNG, "ELNRNG", "Link number out of range"),
#endif
#if defined(EUNATCH)
   ENTRY(EUNATCH, "EUNATCH", "Protocol driver not attached"),
#endif
#if defined(ENOCSI)
   ENTRY(ENOCSI, "ENOCSI", "No CSI structure available"),
#endif
#if defined(EL2HLT)
   ENTRY(EL2HLT, "EL2HLT", "Level 2 halted"),
#endif
#if defined(EDEADLK)
   ENTRY(EDEADLK, "EDEADLK", "Deadlock condition"),
#endif
#if defined(ENOLCK)
   ENTRY(ENOLCK, "ENOLCK", "No record locks available"),
#endif
#if defined(EBADE)
   ENTRY(EBADE, "EBADE", "Invalid exchange"),
#endif
#if defined(EBADR)
   ENTRY(EBADR, "EBADR", "Invalid request descriptor"),
#endif
#if defined(EXFULL)
   ENTRY(EXFULL, "EXFULL", "Exchange full"),
#endif
#if defined(ENOANO)
   ENTRY(ENOANO, "ENOANO", "No anode"),
#endif
#if defined(EBADRQC)
   ENTRY(EBADRQC, "EBADRQC", "Invalid request code"),
#endif
#if defined(EBADSLT)
   ENTRY(EBADSLT, "EBADSLT", "Invalid slot"),
#endif
#if defined(EDEADLOCK)
   ENTRY(EDEADLOCK, "EDEADLOCK", "File locking deadlock error"),
#endif
#if defined(EBFONT)
   ENTRY(EBFONT, "EBFONT", "Bad font file format"),
#endif
#if defined(ENOSTR)
   ENTRY(ENOSTR, "ENOSTR", "Device not a stream"),
#endif
#if defined(ENODATA)
   ENTRY(ENODATA, "ENODATA", "No data available"),
#endif
#if defined(ETIME)
   ENTRY(ETIME, "ETIME", "Timer expired"),
#endif
#if defined(ENOSR)
   ENTRY(ENOSR, "ENOSR", "Out of streams resources"),
#endif
#if defined(ENONET)
   ENTRY(ENONET, "ENONET", "Machine is not on the network"),
#endif
#if defined(ENOPKG)
   ENTRY(ENOPKG, "ENOPKG", "Package not installed"),
#endif
#if defined(EREMOTE)
   ENTRY(EREMOTE, "EREMOTE", "Object is remote"),
#endif
#if defined(ENOLINK)
   ENTRY(ENOLINK, "ENOLINK", "Link has been severed"),
#endif
#if defined(EADV)
   ENTRY(EADV, "EADV", "Advertise error"),
#endif
#if defined(ESRMNT)
   ENTRY(ESRMNT, "ESRMNT", "Srmount error"),
#endif
#if defined(ECOMM)
   ENTRY(ECOMM, "ECOMM", "Communication error on send"),
#endif
#if defined(EPROTO)
   ENTRY(EPROTO, "EPROTO", "Protocol error"),
#endif
#if defined(EMULTIHOP)
   ENTRY(EMULTIHOP, "EMULTIHOP", "Multihop attempted"),
#endif
#if defined(EDOTDOT)
   ENTRY(EDOTDOT, "EDOTDOT", "RFS specific error"),
#endif
#if defined(EBADMSG)
   ENTRY(EBADMSG, "EBADMSG", "Not a data message"),
#endif
#if defined(ENAMETOOLONG)
   ENTRY(ENAMETOOLONG, "ENAMETOOLONG", "File name too long"),
#endif
#if defined(EOVERFLOW)
   ENTRY(EOVERFLOW, "EOVERFLOW", "Value too large for defined data type"),
#endif
#if defined(ENOTUNIQ)
   ENTRY(ENOTUNIQ, "ENOTUNIQ", "Name not unique on network"),
#endif
#if defined(EBADFD)
   ENTRY(EBADFD, "EBADFD", "File descriptor in bad state"),
#endif
#if defined(EREMCHG)
   ENTRY(EREMCHG, "EREMCHG", "Remote address changed"),
#endif
#if defined(ELIBACC)
   ENTRY(ELIBACC, "ELIBACC", "Can not access a needed shared library"),
#endif
#if defined(ELIBBAD)
   ENTRY(ELIBBAD, "ELIBBAD", "Accessing a corrupted shared library"),
#endif
#if defined(ELIBSCN)
   ENTRY(ELIBSCN, "ELIBSCN", ".lib section in a.out corrupted"),
#endif
#if defined(ELIBMAX)
   ENTRY(ELIBMAX, "ELIBMAX", "Attempting to link in too many shared libraries"),
#endif
#if defined(ELIBEXEC)
   ENTRY(ELIBEXEC, "ELIBEXEC", "Cannot exec a shared library directly"),
#endif
#if defined(EILSEQ)
   ENTRY(EILSEQ, "EILSEQ", "Illegal byte sequence"),
#endif
#if defined(ENOSYS)
   ENTRY(ENOSYS, "ENOSYS", "Operation not applicable"),
#endif
#if defined(ELOOP)
   ENTRY(ELOOP, "ELOOP", "Too many symbolic links encountered"),
#endif
#if defined(ERESTART)
   ENTRY(ERESTART, "ERESTART", "Interrupted system call should be restarted"),
#endif
#if defined(ESTRPIPE)
   ENTRY(ESTRPIPE, "ESTRPIPE", "Streams pipe error"),
#endif
#if defined(ENOTEMPTY)
   ENTRY(ENOTEMPTY, "ENOTEMPTY", "Directory not empty"),
#endif
#if defined(EUSERS)
   ENTRY(EUSERS, "EUSERS", "Too many users"),
#endif
#if defined(ENOTSOCK)
   ENTRY(ENOTSOCK, "ENOTSOCK", "Socket operation on non-socket"),
#endif
#if defined(EDESTADDRREQ)
   ENTRY(EDESTADDRREQ, "EDESTADDRREQ", "Destination address required"),
#endif
#if defined(EMSGSIZE)
   ENTRY(EMSGSIZE, "EMSGSIZE", "Message too long"),
#endif
#if defined(EPROTOTYPE)
   ENTRY(EPROTOTYPE, "EPROTOTYPE", "Protocol wrong type for socket"),
#endif
#if defined(ENOPROTOOPT)
   ENTRY(ENOPROTOOPT, "ENOPROTOOPT", "Protocol not available"),
#endif
#if defined(EPROTONOSUPPORT)
   ENTRY(EPROTONOSUPPORT, "EPROTONOSUPPORT", "Protocol not supported"),
#endif
#if defined(ESOCKTNOSUPPORT)
   ENTRY(ESOCKTNOSUPPORT, "ESOCKTNOSUPPORT", "Socket type not supported"),
#endif
#if defined(EOPNOTSUPP)
   ENTRY(EOPNOTSUPP, "EOPNOTSUPP", "Operation not supported on transport endpoint"),
#endif
#if defined(EPFNOSUPPORT)
   ENTRY(EPFNOSUPPORT, "EPFNOSUPPORT", "Protocol family not supported"),
#endif
#if defined(EAFNOSUPPORT)
   ENTRY(EAFNOSUPPORT, "EAFNOSUPPORT", "Address family not supported by protocol"),
#endif
#if defined(EADDRINUSE)
   ENTRY(EADDRINUSE, "EADDRINUSE", "Address already in use"),
#endif
#if defined(EADDRNOTAVAIL)
   ENTRY(EADDRNOTAVAIL, "EADDRNOTAVAIL","Cannot assign requested address"),
#endif
#if defined(ENETDOWN)
   ENTRY(ENETDOWN, "ENETDOWN", "Network is down"),
#endif
#if defined(ENETUNREACH)
   ENTRY(ENETUNREACH, "ENETUNREACH", "Network is unreachable"),
#endif
#if defined(ENETRESET)
   ENTRY(ENETRESET, "ENETRESET", "Network dropped connection because of reset"),
#endif
#if defined(ECONNABORTED)
   ENTRY(ECONNABORTED, "ECONNABORTED", "Software caused connection abort"),
#endif
#if defined(ECONNRESET)
   ENTRY(ECONNRESET, "ECONNRESET", "Connection reset by peer"),
#endif
#if defined(ENOBUFS)
   ENTRY(ENOBUFS, "ENOBUFS", "No buffer space available"),
#endif
#if defined(EISCONN)
   ENTRY(EISCONN, "EISCONN", "Transport endpoint is already connected"),
#endif
#if defined(ENOTCONN)
   ENTRY(ENOTCONN, "ENOTCONN", "Transport endpoint is not connected"),
#endif
#if defined(ESHUTDOWN)
   ENTRY(ESHUTDOWN, "ESHUTDOWN", "Cannot send after transport endpoint shutdown"),
#endif
#if defined(ETOOMANYREFS)
   ENTRY(ETOOMANYREFS, "ETOOMANYREFS", "Too many references: cannot splice"),
#endif
#if defined(ETIMEDOUT)
   ENTRY(ETIMEDOUT, "ETIMEDOUT", "Connection timed out"),
#endif
#if defined(ECONNREFUSED)
   ENTRY(ECONNREFUSED, "ECONNREFUSED", "Connection refused"),
#endif
#if defined(EHOSTDOWN)
   ENTRY(EHOSTDOWN, "EHOSTDOWN", "Host is down"),
#endif
#if defined(EHOSTUNREACH)
   ENTRY(EHOSTUNREACH, "EHOSTUNREACH", "No route to host"),
#endif
#if defined(EALREADY)
   ENTRY(EALREADY, "EALREADY", "Operation already in progress"),
#endif
#if defined(EINPROGRESS)
   ENTRY(EINPROGRESS, "EINPROGRESS", "Operation now in progress"),
#endif
#if defined(ESTALE)
   ENTRY(ESTALE, "ESTALE", "Stale NFS file handle"),
#endif
#if defined(EUCLEAN)
   ENTRY(EUCLEAN, "EUCLEAN", "Structure needs cleaning"),
#endif
#if defined(ENOTNAM)
   ENTRY(ENOTNAM, "ENOTNAM", "Not a XENIX named type file"),
#endif
#if defined(ENAVAIL)
   ENTRY(ENAVAIL, "ENAVAIL", "No XENIX semaphores available"),
#endif
#if defined(EISNAM)
   ENTRY(EISNAM, "EISNAM", "Is a named type file"),
#endif
#if defined(EREMOTEIO)
   ENTRY(EREMOTEIO, "EREMOTEIO", "Remote I/O error"),
#endif
#if defined(EDQUOT)
   ENTRY(EDQUOT, "EDQUOT", "Quota exceeded"),
#endif
#if defined(ENOMEDIUM)
   ENTRY(ENOMEDIUM, "ENOMEDIUM", "No medium found"),
#endif
#if defined(EMEDIUMTYPE)
   ENTRY(EMEDIUMTYPE, "EMEDIUMTYPE", "Wrong medium type"),
#endif
#if defined(ENMFILE)
   ENTRY(ENMFILE, "ENMFILE", "No more files"),
#endif
#if defined(ENOSHARE)
   ENTRY(ENOSHARE, "ENOSHARE", "No such host or network path"),
#endif
#if defined(EPROCLIM)
   ENTRY(EPROCLIM, "EPROCLIM", "Too many processes"),
#endif
   ENTRY(0, 0, 0)
};

   static int  nerr = sizeof(error_table) / sizeof(error_table[0]);
   static char buffer[256];

#ifdef EVMSERR
   /* This is not in the table, because the numeric value of EVMSERR (32767) lies outside the range of sys_errlist[]. */
   if (errno == EVMSERR) return "EVMSERR (32767, VMS-specific error)";
#endif

   const char* restrict msg  = "Unknown error";
   const char* restrict name = "???";

   U_INTERNAL_TRACE("u_getSysError(%p)", len)

   if ((errno > 0) &&
       (errno < nerr)) // check if in range 
      {
#  ifdef HAVE_STRERROR
      msg = strerror(errno);
#  endif

      if (error_table[errno].value == errno)
         {
#     ifndef HAVE_STRERROR
         msg  = error_table[errno].msg;
#     endif
         name = error_table[errno].name;
         }
      else
         {
         int i;

         for (i = 0; i < nerr; ++i)
            {
            if (error_table[i].value == errno)
               {
#           ifndef HAVE_STRERROR
               msg  = error_table[i].msg;
#           endif
               name = error_table[i].name;

               break;
               }
            }
         }
      }

   *len = snprintf(buffer, sizeof(buffer), "%s (%d, %s)", name, errno, msg);

   return buffer;
}

/*
Maps an signal number to an signal message string, the contents of which are implementation defined.
The returned string is only guaranteed to be valid only until the next call to strsignal.
*/

const char* u_getSysSignal(int signo, uint32_t* restrict len)
{
   /*
   Translation table for signal values.  Note that this table is generally only accessed when
   it is used at runtime to initialize signal name and message tables that are indexed by signal value.
   Not all of these signals will exist on all systems.  This table is the only thing that should have to
   be updated as new signal numbers are introduced. It's sort of ugly, but at least its portable.
   */

   struct signal_info {
      int                  value;   /* The numeric value from <signal.h> */
      const char* restrict name;    /* The equivalent symbolic value */
#  ifndef HAVE_STRSIGNAL
      const char* restrict msg;     /* Short message about this value */
#  endif
   };

#undef ENTRY
#ifdef HAVE_STRSIGNAL
#   define ENTRY(value,name,msg) {value,name}
#else
#   define ENTRY(value,name,msg) {value,name,msg}
#endif

   static const struct signal_info signal_table[] = {
#if defined(SIGHUP)
   ENTRY(SIGHUP, "SIGHUP", "Hangup"),
#endif
#if defined(SIGINT)
   ENTRY(SIGINT, "SIGINT", "Interrupt"),
#endif
#if defined(SIGQUIT)
   ENTRY(SIGQUIT, "SIGQUIT", "Quit"),
#endif
#if defined(SIGILL)
   ENTRY(SIGILL, "SIGILL", "Illegal instruction"),
#endif
#if defined(SIGTRAP)
   ENTRY(SIGTRAP, "SIGTRAP", "Trace/breakpoint trap"),
#endif
   /* Put SIGIOT before SIGABRT, so that if SIGIOT == SIGABRT then SIGABRT overrides SIGIOT. SIGABRT is in ANSI and POSIX.1, and SIGIOT isn't. */
#if defined(SIGIOT)
   ENTRY(SIGIOT, "SIGIOT", "IOT trap"),
#endif
#if defined(SIGABRT)
   ENTRY(SIGABRT, "SIGABRT", "Aborted"),
#endif
#if defined(SIGEMT)
   ENTRY(SIGEMT, "SIGEMT", "Emulation trap"),
#endif
#if defined(SIGFPE)
   ENTRY(SIGFPE, "SIGFPE", "Arithmetic exception"),
#endif
#if defined(SIGKILL)
   ENTRY(SIGKILL, "SIGKILL", "Killed"),
#endif
#if defined(SIGBUS)
   ENTRY(SIGBUS, "SIGBUS", "Bus error"),
#endif
#if defined(SIGSEGV)
   ENTRY(SIGSEGV, "SIGSEGV", "Segmentation fault"),
#endif
#if defined(SIGSYS)
   ENTRY(SIGSYS, "SIGSYS", "Bad system call"),
#endif
#if defined(SIGPIPE)
   ENTRY(SIGPIPE, "SIGPIPE", "Broken pipe"),
#endif
#if defined(SIGALRM)
   ENTRY(SIGALRM, "SIGALRM", "Alarm clock"),
#endif
#if defined(SIGTERM)
   ENTRY(SIGTERM, "SIGTERM", "Terminated"),
#endif
#if defined(SIGUSR1)
   ENTRY(SIGUSR1, "SIGUSR1", "User defined signal 1"),
#endif
#if defined(SIGUSR2)
   ENTRY(SIGUSR2, "SIGUSR2", "User defined signal 2"),
#endif
   /* Put SIGCLD before SIGCHLD, so that if SIGCLD == SIGCHLD then SIGCHLD overrides SIGCLD. SIGCHLD is in POXIX.1 */
#if defined(SIGCLD)
   ENTRY(SIGCLD, "SIGCLD", "Child status changed"),
#endif
#if defined(SIGCHLD)
   ENTRY(SIGCHLD, "SIGCHLD", "Child status changed"),
#endif
#if defined(SIGPWR)
   ENTRY(SIGPWR, "SIGPWR", "Power fail/restart"),
#endif
#if defined(SIGWINCH)
   ENTRY(SIGWINCH, "SIGWINCH", "Window size changed"),
#endif
#if defined(SIGURG)
   ENTRY(SIGURG, "SIGURG", "Urgent I/O condition"),
#endif
#if defined(SIGIO)
   /* I/O pending has also been suggested, but is misleading since the signal only happens when the process has asked for it, not everytime I/O is pending */
   ENTRY(SIGIO, "SIGIO", "I/O possible"),
#endif
#if defined(SIGPOLL)
   ENTRY(SIGPOLL, "SIGPOLL", "Pollable event occurred"),
#endif
#if defined(SIGSTKFLT)
   ENTRY(SIGSTKFLT, "SIGSTKFLT", "Stack fault"),
#endif
#if defined(SIGSTOP)
   ENTRY(SIGSTOP, "SIGSTOP", "Stopped (signal)"),
#endif
#if defined(SIGTSTP)
   ENTRY(SIGTSTP, "SIGTSTP", "Stopped (user)"),
#endif
#if defined(SIGCONT)
   ENTRY(SIGCONT, "SIGCONT", "Continued"),
#endif
#if defined(SIGTTIN)
   ENTRY(SIGTTIN, "SIGTTIN", "Stopped (tty input)"),
#endif
#if defined(SIGTTOU)
   ENTRY(SIGTTOU, "SIGTTOU", "Stopped (tty output)"),
#endif
#if defined(SIGVTALRM)
   ENTRY(SIGVTALRM, "SIGVTALRM", "Virtual timer expired"),
#endif
#if defined(SIGPROF)
   ENTRY(SIGPROF, "SIGPROF", "Profiling timer expired"),
#endif
#if defined(SIGXCPU)
   ENTRY(SIGXCPU, "SIGXCPU", "CPU time limit exceeded"),
#endif
#if defined(SIGXFSZ)
   ENTRY(SIGXFSZ, "SIGXFSZ", "File size limit exceeded"),
#endif
#if defined(SIGWIND)
   ENTRY(SIGWIND, "SIGWIND", "SIGWIND"),
#endif
#if defined(SIGPHONE)
   ENTRY(SIGPHONE, "SIGPHONE", "SIGPHONE"),
#endif
#if defined(SIGLOST)
   ENTRY(SIGLOST, "SIGLOST", "Resource lost"),
#endif
#if defined(SIGWAITING)
   ENTRY(SIGWAITING, "SIGWAITING", "Process's LWPs are blocked"),
#endif
#if defined(SIGLWP)
   ENTRY(SIGLWP, "SIGLWP", "Signal LWP"),
#endif
#if defined(SIGDANGER)
   ENTRY(SIGDANGER, "SIGDANGER", "Swap space dangerously low"),
#endif
#if defined(SIGGRANT)
   ENTRY(SIGGRANT, "SIGGRANT", "Monitor mode granted"),
#endif
#if defined(SIGRETRACT)
   ENTRY(SIGRETRACT, "SIGRETRACT", "Need to relinguish monitor mode"),
#endif
#if defined(SIGMSG)
   ENTRY(SIGMSG, "SIGMSG", "Monitor mode data available"),
#endif
#if defined(SIGSOUND)
   ENTRY(SIGSOUND, "SIGSOUND", "Sound completed"),
#endif
#if defined(SIGSAK)
   ENTRY(SIGSAK, "SIGSAK", "Secure attention"),
#endif
   ENTRY(0, 0, 0)
};

   static int  nsig = sizeof(signal_table) / sizeof(signal_table[0]);
   static char buffer[256];

   const char* restrict msg = "Unknown signal";
   const char* restrict name = "???";

   U_INTERNAL_TRACE("u_getSysSignal(%d,%p)", signo, len)

   if ((signo > 0) &&
       (signo < nsig)) /* check if in range */
      {
#  ifdef HAVE_STRSIGNAL
      msg = strsignal(signo);
#  endif

      if (signal_table[signo].value == signo)
         {
#     ifndef HAVE_STRSIGNAL
         msg  = signal_table[signo].msg;
#     endif
         name = signal_table[signo].name;
         }
      else
         {
         int i;

         for (i = 0; i < nsig; ++i)
            {
            if (signal_table[i].value == signo)
               {
#           ifndef HAVE_STRSIGNAL
               msg  = signal_table[i].msg;
#           endif
               name = signal_table[i].name;

               break;
               }
            }
         }
      }

   *len = snprintf(buffer, sizeof(buffer), "%s (%d, %s)", name, signo, msg);

   return buffer;
}

#undef  ENTRY
#define ENTRY(value,name,msg) {value,name,msg}

const char* u_getExitStatus(int exitno, uint32_t* restrict len)
{
   /* Translation table for exit status codes for system programs */

   struct exit_value_info {
      int                  value; /* The numeric value from <sysexits.h> */
      const char* restrict name;  /* The equivalent symbolic value */
      const char* restrict msg;   /* Short message about this value */
   };

   static const struct exit_value_info exit_value_table[] = {
#if defined(EX_USAGE)
   ENTRY(EX_USAGE, "EX_USAGE", "command line usage error"),
#endif
#if defined(EX_DATAERR)
   ENTRY(EX_DATAERR, "EX_DATAERR", "data format error"),
#endif
#if defined(EX_NOINPUT)
   ENTRY(EX_NOINPUT, "EX_NOINPUT", "cannot open input"),
#endif
#if defined(EX_NOUSER)
   ENTRY(EX_NOUSER, "EX_NOUSER", "addresse unknown"),
#endif
#if defined(EX_NOHOST)
   ENTRY(EX_NOHOST, "EX_NOHOST", "host name unknown"),
#endif
#if defined(EX_UNAVAILABLE)
   ENTRY(EX_UNAVAILABLE, "EX_UNAVAILABLE", "service unavailable"),
#endif
#if defined(EX_SOFTWARE)
   ENTRY(EX_SOFTWARE, "EX_SOFTWARE", "internal software error"),
#endif
#if defined(EX_OSERR)
   ENTRY(EX_OSERR, "EX_OSERR", "system error (e.g., can't fork)"),
#endif
#if defined(EX_OSFILE)
   ENTRY(EX_OSFILE, "EX_OSFILE", "critical OS file missing"),
#endif
#if defined(EX_CANTCREAT)
   ENTRY(EX_CANTCREAT, "EX_CANTCREAT", "can't create (user) output file"),
#endif
#if defined(EX_IOERR)
   ENTRY(EX_IOERR, "EX_IOERR", "input/output error"),
#endif
#if defined(EX_TEMPFAIL)
   ENTRY(EX_TEMPFAIL, "EX_TEMPFAIL", "temp failure; user is invited to retry"),
#endif
#if defined(EX_PROTOCOL)
   ENTRY(EX_PROTOCOL, "EX_PROTOCOL", "remote error in protocol"),
#endif
#if defined(EX_NOPERM)
   ENTRY(EX_NOPERM, "EX_NOPERM", "permission denied"),
#endif
#if defined(EX_CONFIG)
   ENTRY(EX_CONFIG, "EX_CONFIG", "configuration error"),
#endif
   ENTRY(0, 0, 0)
};

   static int  nval = sizeof(exit_value_table) / sizeof(exit_value_table[0]);
   static char buffer[256];

   const char* restrict msg;
   const char* restrict name;

   U_INTERNAL_TRACE("u_getExitStatus(%d,%p)", exitno, len)

   if (exitno == 0)
      {
      msg  = "successful termination";
      name = "EX_OK";
      }
   else
      {
      msg  = "Unknown value";
      name = "???";
      }

   if ((exitno >= EX__BASE) &&
       (exitno <= EX__MAX)) // check if in range
      {
      int _index = exitno - EX__BASE;

      if (exit_value_table[_index].value == exitno)
         {
         msg  = exit_value_table[_index].msg;
         name = exit_value_table[_index].name;
         }
      else
         {
         int i;

         for (i = 0; i < nval; ++i)
            {
            if (exit_value_table[i].value == exitno)
               {
               msg  = exit_value_table[i].msg;
               name = exit_value_table[i].name;

               break;
               }
            }
         }
      }

   *len = snprintf(buffer, 256, "%s (%d, %s)", name, exitno, msg);

   return buffer;
}

void u_execOnExit(void)
{
   char* cmd_on_exit = getenv("EXEC_ON_EXIT");

   U_INTERNAL_TRACE("u_execOnExit()")

   if ( cmd_on_exit &&
       *cmd_on_exit)
      {
      // -------------------------------------------------------------------------
      // Example: EXEC_ON_EXIT command for to dump the execution stack
      // -------------------------------------------------------------------------
      // value: /utility/stack_dump.sh
      // -------------------------------------------------------------------------

      char command[U_PATH_MAX];

      (void) u_snprintf(command, sizeof(command), "%s %s %P %N", cmd_on_exit, u_progpath);

      U_INTERNAL_PRINT("command = %s", command)

      if (u_is_tty) U_WARNING("EXEC_ON_EXIT<%Won%W>: COMMAND=%W%S%W\n", GREEN, YELLOW, CYAN, YELLOW, command);

      (void) system(command);
      }
}
