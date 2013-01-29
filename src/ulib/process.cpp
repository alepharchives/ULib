// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    process.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/base/utility.h>

#include <ulib/process.h>
#include <ulib/utility/interrupt.h>

#ifdef __MINGW32__
HANDLE               UProcess::hFile[6];
HANDLE               UProcess::hChildIn;
HANDLE               UProcess::hChildOut;
HANDLE               UProcess::hChildErr;
STARTUPINFO          UProcess::aStartupInfo;
PROCESS_INFORMATION  UProcess::aProcessInformation;
#else
#  include <sys/wait.h>
#endif

#include <errno.h>

int UProcess::filedes[6];

// services for EXEC

void UProcess::kill(pid_t pid, int sig)
{
   U_TRACE(1, "UProcess::kill(%d,%d)", pid, sig)

   (void) U_SYSCALL(kill, "%d,%d", pid, sig);
}

void UProcess::nice(int inc)
{
   U_TRACE(1, "UProcess::nice(%d)", inc)

   (void) U_SYSCALL(nice, "%d", inc);
}

void UProcess::setProcessGroup(pid_t pid, pid_t pgid)
{
   U_TRACE(1, "UProcess::setProcessGroup(%d,%d)", pid, pgid)

#ifndef __MINGW32__
   (void) U_SYSCALL(setpgid, "%d,%d", pid, pgid);
#endif
}

bool UProcess::fork()
{
   U_TRACE(1, "UProcess::fork()")

   U_CHECK_MEMORY

   _pid = U_FORK();

   if (child()) u_setPid();

   running = (_pid != -1);

   U_RETURN(running);
}

// inlining failed in call to 'UProcess::setStdInOutErr(bool, bool, bool)': call is unlikely and code size would grow

U_NO_EXPORT void UProcess::setStdInOutErr(bool fd_stdin, bool fd_stdout, bool fd_stderr)
{
   U_TRACE(1, "UProcess::setStdInOutErr(%b,%b,%b)", fd_stdin, fd_stdout, fd_stderr)

#ifdef __MINGW32__
   HANDLE hProcess = GetCurrentProcess();
#endif

   // check if we write to STDIN

   if (fd_stdin)
      {
#  ifdef __MINGW32__
      if (hFile[1]) // Created parent-output pipe...
         {
         hChildIn = hFile[0];

         // Duplicating as inheritable child-input pipe
         // -------------------------------------------
         // (void) U_SYSCALL(DuplicateHandle, "%p,%p,%p,%p,%lu,%b,%lu", hProcess, hFile[0], hProcess, &hChildIn, 0, TRUE, DUPLICATE_SAME_ACCESS);
         // (void) U_SYSCALL(    CloseHandle, "%p",                               hFile[0]);
         }
      else
         {
         hChildIn = (HANDLE)_get_osfhandle(filedes[0]);
         }
#  else
      U_INTERNAL_ASSERT_MAJOR(filedes[0],STDERR_FILENO)

      (void) U_SYSCALL(dup2, "%d,%d", filedes[0], STDIN_FILENO);

      U_INTERNAL_ASSERT_EQUALS(::fcntl(STDIN_FILENO,F_GETFD,FD_CLOEXEC), 0)
#  endif
      }

   // check if we read from STDOUT

   if (fd_stdout)
      {
#  ifdef __MINGW32__
      if (hFile[2]) // Created parent-input pipe...
         {
         hChildOut = hFile[3];

         // Duplicating as inheritable child-output pipe
         // -------------------------------------------
         // (void) U_SYSCALL(DuplicateHandle, "%p,%p,%p,%p,%lu,%b,%lu", hProcess, hFile[3], hProcess, &hChildOut, 0, TRUE, DUPLICATE_SAME_ACCESS);
         // (void) U_SYSCALL(    CloseHandle, "%p",                               hFile[3]);
         }
      else
         {
         hChildOut = (HANDLE)_get_osfhandle(filedes[3]);
         }
#  else
      U_INTERNAL_ASSERT_MAJOR(filedes[3],STDOUT_FILENO)

      (void) U_SYSCALL(dup2, "%d,%d", filedes[3], STDOUT_FILENO);

      U_INTERNAL_ASSERT_EQUALS(::fcntl(STDOUT_FILENO,F_GETFD,FD_CLOEXEC), 0)
#  endif
      }

   // check if we read from STDERR

   if (fd_stderr)
      {
#  ifdef __MINGW32__
      if (hFile[4]) // Created parent-input pipe...
         {
         hChildErr = hFile[5];

         // Duplicating as inheritable child-output pipe
         // -------------------------------------------
         // (void) U_SYSCALL(DuplicateHandle, "%p,%p,%p,%p,%lu,%b,%lu", hProcess, hFile[5], hProcess, &hChildErr, 0, TRUE, DUPLICATE_SAME_ACCESS);
         // (void) U_SYSCALL(    CloseHandle, "%p",                               hFile[5]);
         }
      else
         {
         hChildErr = (HANDLE)_get_osfhandle(filedes[5]);
         }
#  else
      U_INTERNAL_ASSERT(filedes[5] >= STDOUT_FILENO)

      (void) U_SYSCALL(dup2, "%d,%d", filedes[5], STDERR_FILENO);

      U_INTERNAL_ASSERT_EQUALS(::fcntl(STDERR_FILENO,F_GETFD,FD_CLOEXEC), 0)
#  endif
      }

#ifdef __MINGW32__
   CloseHandle(hProcess);
#else
   if (fd_stdin)
      {
      U_INTERNAL_DUMP("filedes[0,1] = { %d, %d }", filedes[0], filedes[1])

                                      (void) U_SYSCALL(close, "%d", filedes[0]);
      if (filedes[1] > STDERR_FILENO) (void) U_SYSCALL(close, "%d", filedes[1]);
      }

   if (fd_stdout)
      {
      U_INTERNAL_DUMP("filedes[2,3] = { %d, %d }", filedes[2], filedes[3])

                                       (void) U_SYSCALL(close, "%d", filedes[3]);
      if (filedes[2] > STDERR_FILENO)  (void) U_SYSCALL(close, "%d", filedes[2]);
      }

   if (fd_stderr)
      {
      U_INTERNAL_DUMP("filedes[4,5] = { %d, %d }", filedes[4], filedes[5])

                                       (void) U_SYSCALL(close, "%d", filedes[5]);
      if (filedes[4] > STDERR_FILENO)  (void) U_SYSCALL(close, "%d", filedes[4]);
      }
#endif
}

#ifdef __MINGW32__

void UProcess::pipe(int fdp)
{
   U_TRACE(1, "UProcess::pipe(%d)", fdp)

   U_INTERNAL_ASSERT_RANGE(STDIN_FILENO,fdp,STDERR_FILENO)

   int fdn = (fdp * 2); // filedes[fdn] is for READING, filedes[fdn+1] is for WRITING

   if (U_SYSCALL(CreatePipe, "%p,%p,%p,%lu", hFile+fdn, hFile+fdn+1, &sec_none, 0))
      {
      U_INTERNAL_DUMP("hFile[%d,%d] = { %p, %p }", fdn, fdn+1, hFile[fdn], hFile[fdn+1])
      }
   else
      {
      U_INTERNAL_DUMP("$R", "CreatePipe()")
      }
}

#else

void UProcess::pipe(int fdp)
{
   U_TRACE(1, "UProcess::pipe(%d)", fdp)

   // pipe() creates a pair of file descriptors, pointing to a pipe inode, and places them in the array pointed to by fds.

   U_INTERNAL_ASSERT_RANGE(STDIN_FILENO,fdp,STDERR_FILENO)

   int* fds = filedes + (fdp * 2); // fds[0] is for READING, fds[1] is for WRITING

   (void) U_SYSCALL(pipe, "%p", fds);

   U_INTERNAL_DUMP("filedes[%d,%d] = { %d, %d }", (fdp * 2), (fdp * 2) + 1, fds[0], fds[1])
}

#endif

#ifdef __MINGW32__

static inline BOOL is_console(HANDLE h) { return h != INVALID_HANDLE_VALUE && ((ULONG_PTR)h & 3) == 3; }

pid_t UProcess::execute(const char* pathname, char* argv[], char* envp[], bool fd_stdin, bool fd_stdout, bool fd_stderr)
{
   U_TRACE(1, "UProcess::execute(%S,%p,%p,%b,%b,%b)", pathname, argv, envp, fd_stdin, fd_stdout, fd_stderr)

   U_INTERNAL_ASSERT_POINTER(argv)
   U_DUMP_EXEC(argv, envp)
   U_INTERNAL_ASSERT_EQUALS(strcmp(argv[0],u_basename(pathname)),0)

   (void) U_SYSCALL(memset, "%p,%d,%lu",        &aStartupInfo, 0, sizeof(STARTUPINFO));
   (void) U_SYSCALL(memset, "%p,%d,%lu", &aProcessInformation, 0, sizeof(PROCESS_INFORMATION));

   /*
   typedef struct _STARTUPINFO {
   DWORD cb;            // Size of the structure, in bytes
   LPTSTR lpReserved;
   LPTSTR lpDesktop;
   LPTSTR lpTitle;
   DWORD dwX;
   DWORD dwY;
   DWORD dwXSize;
   DWORD dwYSize;
   DWORD dwXCountChars;
   DWORD dwYCountChars;
   DWORD dwFillAttribute;
   DWORD dwFlags;
   WORD wShowWindow;
   WORD cbReserved2;
   LPBYTE lpReserved2;
   HANDLE hStdInput;
   HANDLE hStdOutput;
   HANDLE hStdError;
   } STARTUPINFO, *LPSTARTUPINFO;
   */

   aStartupInfo.cb          = sizeof(STARTUPINFO);
   aStartupInfo.dwFlags     = STARTF_USESHOWWINDOW;
   aStartupInfo.wShowWindow = SW_SHOWNORMAL;

   // STARTF_USESTDHANDLES - Sets the standard input, standard output, and standard error handles for the process
   // to the handles specified in the hStdInput, hStdOutput, and hStdError members of the STARTUPINFO structure.
   // For this to work properly, the handles must be inheritable and the CreateProcess function's fInheritHandles
   // parameter must be set to TRUE. If this value is not specified, the hStdInput, hStdOutput, and hStdError
   // members of the STARTUPINFO structure are ignored.

   if (fd_stdin || fd_stdout || fd_stderr)
      {
      aStartupInfo.dwFlags |= STARTF_USESTDHANDLES; // Tell the new process to use our std handles

      setStdInOutErr(fd_stdin, fd_stdout, fd_stderr);
      }

   aStartupInfo.hStdInput  = (fd_stdin  ? hChildIn  : GetStdHandle(STD_INPUT_HANDLE));
   aStartupInfo.hStdOutput = (fd_stdout ? hChildOut : GetStdHandle(STD_OUTPUT_HANDLE));
   aStartupInfo.hStdError  = (fd_stderr ? hChildErr : GetStdHandle(STD_ERROR_HANDLE));

   U_INTERNAL_DUMP("hStdInput(%b) = %p hStdOutput(%b) = %p hStdError(%b) = %p", is_console(aStartupInfo.hStdInput),  aStartupInfo.hStdInput,
                                                                                is_console(aStartupInfo.hStdOutput), aStartupInfo.hStdOutput,
                                                                                is_console(aStartupInfo.hStdError),  aStartupInfo.hStdError)

   char* w32_shell = (U_STRNEQ(argv[0], "sh.exe") ? (char*)pathname : 0); 

   U_INTERNAL_DUMP("w32_shell = %S", w32_shell)

   int index = 0;
   char buffer1[4096];
   char* w32_cmd = (char*)pathname;

   if (argv[1])
      {
      bool flag;
      w32_cmd = buffer1;
      int len = u__strlen(pathname);

      if (len)
         {
         index = len;

         U__MEMCPY(w32_cmd, pathname, len);
         }

      for (int i = 1; argv[i]; ++i)
         {
         w32_cmd[index++] = ' ';

         len = u__strlen(argv[i]);

         if (len)
            {
            U_INTERNAL_ASSERT_MINOR(index+len+3,4096)

            flag = (strchr(argv[i], ' ') != 0);

            if (flag) w32_cmd[index++] = '"';

            U__MEMCPY(w32_cmd+index, argv[i], len);

            index += len;

            if (flag) w32_cmd[index++] = '"';
            }
         }

      w32_cmd[index] = '\0';
      }

   U_INTERNAL_DUMP("w32_cmd(%d) = %.*S", index, index, w32_cmd)

   char* w32_envp = 0;
   char buffer2[32000] = { '\0' };

   if (envp)
      {
      index    = 0;
      w32_envp = buffer2;

      for (int len, i = 0; envp[i]; ++i, ++index)
         {
         len = u__strlen(envp[i]);

         if (len)
            {
            U_INTERNAL_ASSERT_MINOR(index+len+1,32000)

            U__MEMCPY(w32_envp+index, envp[i], len);

            index += len;

            w32_envp[index] = '\0';
            }
         }

      w32_envp[index+1] = '\0';

      U_INTERNAL_DUMP("w32_envp(%d) = %#.*S", index, index+1, w32_envp)
      }

   pid_t pid;
   BOOL fRet = U_SYSCALL(CreateProcessA, "%S,%S,%p,%p,%b,%lu,%p,%p,%p,%p",
                             w32_shell,               // No module name (use command line)
                             w32_cmd,                 // Command line
                             &sec_none,               // Default process security attributes
                             &sec_none,               // Default  thread security attributes
                             sec_none.bInheritHandle, // inherit handles from the parent
                             DETACHED_PROCESS,        // the new process does not inherit its parent's console
                             w32_envp,                // if NULL use the same environment as the parent
                             0,                       // Launch in the current directory
                             &aStartupInfo,           // Startup Information
                             &aProcessInformation);   // Process information stored upon return

   if (fRet)
      {
      /*
      typedef struct _PROCESS_INFORMATION {
      HANDLE hProcess;
      HANDLE hThread;
      DWORD dwProcessId;
      DWORD dwThreadId;
      } PROCESS_INFORMATION;
      */

      U_INTERNAL_DUMP("dwProcessId = %p hProcess = %p hThread = %p", aProcessInformation.dwProcessId,
                                                                     aProcessInformation.hProcess,
                                                                     aProcessInformation.hThread)

      u_hProcess = aProcessInformation.hProcess;

      pid = (pid_t) aProcessInformation.dwProcessId;

      (void) U_SYSCALL(CloseHandle, "%p", aProcessInformation.hThread);
      }
   else
      {
      U_INTERNAL_DUMP("$R", "CreateProcess()")

      pid = -1;
      }

   /* associate handle to filedes */

   if (fd_stdin && hFile[1])
      {
      filedes[0] = _open_osfhandle((long)hChildIn, (_O_RDONLY | O_BINARY));
      filedes[1] = _open_osfhandle((long)hFile[1], (_O_WRONLY | O_BINARY));

      U_INTERNAL_DUMP("filedes[0,1] = { %d, %d }", filedes[0], filedes[1])
      }

   if (fd_stdout && hFile[2])
      {
      filedes[2] = _open_osfhandle((long)hFile[2],  (_O_RDONLY | O_BINARY));
      filedes[3] = _open_osfhandle((long)hChildOut, (_O_WRONLY | O_BINARY));

      U_INTERNAL_DUMP("filedes[2,3] = { %d, %d }", filedes[2], filedes[3])
      }

   if (fd_stderr && hFile[4])
      {
      filedes[4] = _open_osfhandle((long)hFile[4],  (_O_RDONLY | O_BINARY));
      filedes[5] = _open_osfhandle((long)hChildErr, (_O_WRONLY | O_BINARY));

      U_INTERNAL_DUMP("filedes[4,5] = { %d, %d }", filedes[4], filedes[5])
      }

   hChildIn = hChildOut = hChildErr = 0;

   (void) U_SYSCALL(memset, "%p,%d,%lu", hFile, 0, sizeof(hFile));

   U_RETURN(pid);
}

#else

pid_t UProcess::execute(const char* pathname, char* argv[], char* envp[], bool fd_stdin, bool fd_stdout, bool fd_stderr)
{
   U_TRACE(1, "UProcess::execute(%S,%p,%p,%b,%b,%b)", pathname, argv, envp, fd_stdin, fd_stdout, fd_stderr)

   U_INTERNAL_ASSERT_POINTER(argv)
   U_DUMP_EXEC(argv, envp)
   U_INTERNAL_ASSERT_EQUALS(strcmp(u_basename(pathname), argv[0]),0)

   pid_t pid = U_VFORK();

   if (pid == 0) // child
      {
      setStdInOutErr(fd_stdin, fd_stdout, fd_stderr);

      U_EXEC(pathname, argv, envp);
      }

   // parent

   if (u_exec_failed) U_RETURN(-1);

   U_RETURN(pid);
}

#endif

int UProcess::waitpid(pid_t pid, int* _status, int options)
{
   U_TRACE(1, "UProcess::waitpid(%d,%p,%d)", pid, _status, options)

   int result;

loop:
   result = U_SYSCALL(waitpid, "%d,%p,%d", pid, _status, options);

   if (result == -1 && UInterrupt::checkForEventSignalPending()) goto loop;

   // errno == ECHILD: if the process specified in pid does not exist or is not a child of the calling process...

#if DEBUG
   if (_status) U_INTERNAL_DUMP("status = %d", *_status)
#endif

   U_RETURN(result);
}

int UProcess::waitAll()
{
   U_TRACE(1, "UProcess::waitAll()")

   wait();

   int result = (status ? U_FAILED_ALL    // (status != 0) -> failed
                        : U_FAILED_NONE); // (status == 0) -> success

   while (UProcess::waitpid(-1, &status, 0) > 0)
      {
      if ((status == 0 && result == U_FAILED_ALL) || // (status == 0) -> success
          (status != 0 && result == U_FAILED_NONE))  // (status != 0) -> failed
         {
         result = U_FAILED_SOME;
         }

      U_DUMP("result = %b status = %d, %S", result, status, exitInfo())
      }

   U_RETURN(result);
}

char* UProcess::exitInfo(int _status)
{
   U_TRACE(0, "UProcess::exitInfo(%d)", _status)

   uint32_t n = 0;

   static char buffer[128];

   if (WIFEXITED(_status))
      {
      n = u__snprintf(buffer, sizeof(buffer), "Exit %d", WEXITSTATUS(_status));
      }
   else if (WIFSIGNALED(_status))
      {
#  ifndef WCOREDUMP
#  define WCOREDUMP(status) ((status) & 0200) // settimo bit
#  endif
      n = u__snprintf(buffer, sizeof(buffer), "Signal %Y%s", WTERMSIG(_status), (WCOREDUMP(_status) ? " - core dumped" : ""));
      }
   else if (WIFSTOPPED(_status))
      {
      n = u__snprintf(buffer, sizeof(buffer), "Signal %Y", WSTOPSIG(_status));
      }
#  ifndef WIFCONTINUED
#  define WIFCONTINUED(status)  ((status) == 0xffff)
#  endif
   else if (WIFCONTINUED(_status))
      {
      U__MEMCPY(buffer, "SIGCONT", (n = U_CONSTANT_SIZE("SIGCONT")));
      }

   buffer[n] = '\0';

   U_RETURN(buffer);
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UProcess::dump(bool reset) const
{
   U_CHECK_MEMORY

   *UObjectIO::os << "pid             " << _pid    << '\n'
                  << "status          " << status  << '\n' 
                  << "running         " << running;

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}
#endif
