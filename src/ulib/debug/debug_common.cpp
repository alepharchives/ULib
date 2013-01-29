// ============================================================================
//
// = LIBRARY
//    ulibdbg - c++ library
//
// = FILENAME
//    debug_common.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================
 
#include <ulib/debug/trace.h>
#include <ulib/debug/error.h>
#include <ulib/debug/common.h>
#include <ulib/debug/objectDB.h>

#ifdef __MINGW32__
#  include <process.h>
#else
#  include <pwd.h>
#  include <sys/uio.h>
#  include <sys/resource.h>
#endif

#ifdef HAVE_SYSEXITS_H
#  include <sysexits.h>
#else
#  include <ulib/base/replace/sysexits.h>
#endif

#ifdef ENABLE_MEMPOOL
#  include <ulib/internal/memory_pool.h>
#endif

#include <fstream>
#include <errno.h>

static const char* __restrict__ uid = "@(#) " PACKAGE_NAME " " ULIB_VERSION " " PLATFORM_VAR " (" __DATE__ ")";

extern "C" void U_EXPORT u_debug_at_exit(void)
{
   U_INTERNAL_TRACE("u_debug_at_exit()")

   // NB: if error in this code maybe recursion occurs...

   if (u_recursion == false)
      {
      u_recursion = true;

#if defined(ENABLE_MEMPOOL) && defined(__linux__)
      char name[128];

      (void) u__snprintf(name, sizeof(name), "mempool.%N.%P", 0);

      std::ofstream of(name);

      UMemoryPool::printInfo(of);
#  endif

      UError::stackDump();

      UObjectDB::close();

      u_trace_close();
      }
}

void U_EXPORT u_debug_init(void)
{
   U_INTERNAL_TRACE("u_debug_init()")

   U_MESSAGE("DEBUG MODE (pid %W%P%W) - %s%W", CYAN, YELLOW, uid + 5, RESET); // print program mode and info for ULib...

   USimulationError::init();
          UObjectDB::init(true, true);

   // controllo se sono avvenute precedenti creazioni di oggetti globali
   // che possono avere forzato l'inizializzazione del file di trace...

   u_trace_check_init();
}

// set_memlimit() uses setrlimit() to restrict dynamic memory allocation.
// The argument to set_memlimit() is the limit in megabytes (a floating-point number).

void U_EXPORT u_debug_set_memlimit(float size)
{
   U_TRACE(1, "u_debug_set_memlimit(%f)", size)

   struct rlimit r;
   r.rlim_cur = (rlim_t)(size * 1048576);

   // Heap size, seems to be common.
   (void) U_SYSCALL(setrlimit, "%d,%p", RLIMIT_DATA, &r);

   // Size of stack segment
   (void) U_SYSCALL(setrlimit, "%d,%p", RLIMIT_STACK, &r);

#ifdef RLIMIT_RSS
   // Resident set size.
   // This affects swapping; processes that are exceeding their
   // resident set size will be more likely to have physical memory
   // taken from them.
   (void) U_SYSCALL(setrlimit, "%d,%p", RLIMIT_RSS, &r);
#endif

#ifdef RLIMIT_VMEM
   // Mapped memory (brk + mmap).
   (void) U_SYSCALL(setrlimit, "%d,%p", RLIMIT_VMEM, &r);
#endif

#ifdef RLIMIT_AS
   // Address space limit.
   (void) U_SYSCALL(setrlimit, "%d,%p", RLIMIT_AS, &r);
#endif
}

static bool fork_called;

pid_t U_EXPORT u_debug_fork(pid_t _pid)
{
   U_INTERNAL_TRACE("u_debug_fork(%d)", _pid)

   if (_pid == 0) // child
      {
      u_setPid();

      U_MESSAGE("DEBUG MODE (pid %W%P%W) - %s%W", CYAN, YELLOW, uid + 5, RESET); // print program mode and info for ULib...

                                u_trace_initFork();
      if (UObjectDB::fd > 0) UObjectDB::initFork();
      }

   if (u_trace_isActive(1))
      {
      char buffer[32];

      u_trace_write(buffer, sprintf(buffer, "::fork() = %d", _pid));
      }

   fork_called = true;

   return _pid;
}

pid_t U_EXPORT u_debug_vfork(pid_t _pid)
{
   U_INTERNAL_TRACE("u_debug_vfork(%d)", _pid)

   // NB: same address space...  Parent process execution stopped until the child calls exec() or exit()

   if (u_trace_isActive(1))
      {
      static char buffer[32];

      if         (_pid == 0) u_trace_write(U_CONSTANT_TO_PARAM("..........CHILD..........")); // child
      else // if (_pid  > 0)
         {
         if (u_exec_failed == false)
            {
            struct iovec iov[1] = { { (caddr_t)"\n", 1 } };

            u_trace_writev(iov, 1);
            }

         u_trace_write(U_CONSTANT_TO_PARAM("..........PARENT.........")); // parent
         }

      u_trace_write(buffer, sprintf(buffer, "::vfork() = %d", _pid));
      }

   fork_called = false;

   return _pid;
}

void U_EXPORT u_debug_exit(int exit_value)
{
   U_INTERNAL_TRACE("u_debug_exit(%d)", exit_value)

   if (u_trace_isActive(1))
      {
      char buffer[32];

      u_trace_write(buffer, sprintf(buffer, "::exit(%d)", exit_value));
      }
}

__noreturn void U_EXPORT u_debug_exec(const char* pathname, char* const argv[], char* const envp[])
{
   U_INTERNAL_TRACE("u_debug_exec(%s,%p,%p)", pathname, argv, envp)

   char buffer[1024];
   bool flag_trace_active = false;
   struct iovec iov[3] = { { (caddr_t)u_trace_tab, u_trace_num_tab },
                           { (caddr_t)buffer,                    0 },
                           { (caddr_t)"\n",                      1 } };

   iov[1].iov_len = u__snprintf(buffer, sizeof(buffer), "::execve(%S,%p,%p)", pathname, argv, envp);

   if (u_trace_isActive(1))
      {
      flag_trace_active = true;

      u_trace_writev(iov, 2);
      }

   if (fork_called)
      {
      if (UObjectDB::fd > 0) UObjectDB::close();
      if (u_trace_fd    > 0)    u_trace_close();
      }

   u_exec_failed = false;

   (void) ::execve(pathname, argv, envp);

   u_exec_failed = true;

   if (flag_trace_active == false)
      {
      char _buffer[64];
      uint32_t bytes_written = u__snprintf(_buffer, sizeof(_buffer), "%W%N%W: %WWARNING: %W",BRIGHTCYAN,RESET,YELLOW,RESET);

      (void) lseek(u_printf_fileno, 0, SEEK_END);

      (void) write(u_printf_fileno, _buffer, bytes_written);
      (void) write(u_printf_fileno,  buffer, iov[1].iov_len);
      }

   iov[1].iov_len = u__snprintf(buffer, sizeof(buffer), " = -1%R", 0); // NB: the last argument (0) is necessary...

   if (flag_trace_active == false)
      {
      (void) write(u_printf_fileno, buffer,          iov[1].iov_len);
      (void) write(u_printf_fileno, iov[2].iov_base, iov[2].iov_len);
      }
   else
      {
      if (fork_called) u_trace_init(false, false, true);

      u_trace_writev(iov+1, 2);

      iov[1].iov_len = u__snprintf(buffer, sizeof(buffer), "::_exit(%d)", EX_UNAVAILABLE);

      u_trace_writev(iov, 3);

      if (fork_called) u_trace_close();
      }

   ::_exit(EX_UNAVAILABLE);
}
