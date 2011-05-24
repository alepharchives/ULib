// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    process.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_PROCESS_H
#define ULIB_PROCESS_H 1

#include <ulib/internal/common.h>

#ifdef HAVE_SYSEXITS_H
#  include <sysexits.h>
#else
#  include <ulib/base/replace/sysexits.h>
#endif

#if defined(__CYGWIN__) || defined(__APPLE__)
#  include <sys/wait.h>
#endif

#ifndef __MINGW32__
#  include <sys/socket.h>
#endif

#define U_FAILED_NONE  0
#define U_FAILED_SOME  2
#define U_FAILED_ALL   3

class UCommand;

class U_EXPORT UProcess {
public:

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   UProcess()
      {
      U_TRACE_REGISTER_OBJECT(0, UProcess, "")

      _pid    = (pid_t) -1;
      running = false;
      }

   ~UProcess()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UProcess)
      }

   // FORK

   bool fork();

   pid_t pid() const   { return _pid; }

   bool child() const  { return (_pid == 0); }
   bool parent() const { return (_pid >  0); }

   // WAIT

   void wait()
      {
      U_TRACE(1, "UProcess::wait()")

      U_CHECK_MEMORY

      if (running)
         {
         waitpid(_pid, &status, 0);

         running = false;
         }

      U_DUMP("status = %d, %S", status, exitInfo())
      }

   int waitAll();

   static int waitpid(pid_t pid = -1, int* status = 0, int options = WNOHANG);

   // STATUS CHILD

   static char* exitInfo(int status);

          char* exitInfo() const { return exitInfo(status); }

   static int exitValue(int _status)
      {
      U_TRACE(0, "UProcess::exitValue(%d)", _status)

      int exit_value = (WIFEXITED(_status)
                      ? WEXITSTATUS(_status)
                      : (WTERMSIG(_status) << 8));

      U_RETURN(exit_value);
      }

   int exitValue() const { return exitValue(status); }

   // Inter Process Comunication (IPC)

   static int filedes[6]; // 3 serie di pipe to manage file descriptors for child I/O redirection...

   static void pipe(int fdp);

   // services for EXEC

   static void nice(int inc);
   static void kill(pid_t pid, int sig);
   static void setProcessGroup(pid_t pid = 0, pid_t pgid = 0);

   // exec with internal vfork() with management of file descriptors for child I/O redirection...

   static pid_t execute(const char* pathname, char* argv[], char* envp[], bool fd_stdin, bool fd_stdout, bool fd_stderr);

#ifdef DEBUG
   const char* dump(bool) const;
#endif

protected:
   pid_t _pid;
   int status;
   bool running;

#ifdef __MINGW32__
   static STARTUPINFO aStartupInfo;
   static PROCESS_INFORMATION aProcessInformation;
   static HANDLE hFile[6], hChildIn, hChildOut, hChildErr;
#endif

   static void setStdInOutErr(bool fd_stdin, bool fd_stdout, bool fd_stderr) U_NO_EXPORT;

private:
   UProcess(const UProcess&)            {}
   UProcess& operator=(const UProcess&) { return *this; }

   friend class UCommand;
};

#endif
