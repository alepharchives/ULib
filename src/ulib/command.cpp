// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//   command.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/file.h>
#include <ulib/command.h>
#include <ulib/process.h>
#include <ulib/timeval.h>
#include <ulib/notifier.h>
#include <ulib/utility/services.h>
#include <ulib/utility/string_ext.h>

int   UCommand::status;
int   UCommand::exit_value;
int   UCommand::timeoutMS = -1;
pid_t UCommand::pid;

void UCommand::freeCommand()
{
   U_TRACE(0, "UCommand::freeCommand()")

   if (pathcmd)
      {
      U_SYSCALL_VOID(free, "%p", pathcmd);

      pathcmd = 0;
      }

   if (argv_exec)
      {
      U_INTERNAL_ASSERT_MAJOR(ncmd, 0)

      // NB: considera che u_splitCommand parte da 1 e null terminator...

      U_FREE_VECTOR(argv_exec, 1+ncmd+1 + U_ADD_ARGS, char);

      argv_exec = 0;
      }
}

void UCommand::freeEnvironment()
{
   U_TRACE(0, "UCommand::freeEnvironment()")

   if (envp_exec)
      {
      U_INTERNAL_ASSERT_MAJOR(nenv, 0)

      // NB: considera null terminator...

      U_FREE_VECTOR(envp_exec, nenv+1, char);

      envp_exec = 0;
      }
}

void UCommand::reset(const UString* penv)
{
   U_TRACE(0, "UCommand::reset(%p)", penv)

   freeEnvironment();

   envp = 0;
   nenv = 0;

   if (penv)
      {
      environment = *penv;
      }
   else
      {
      freeCommand();

      ncmd = nfile = 0;
      }
}

void UCommand::setCommand()
{
   U_TRACE(0, "UCommand::setCommand()")

   U_ASSERT(command.empty() == false)

   command.duplicate();

   freeCommand();

   char* argv[U_MAX_ARGS];
   char buffer[U_PATH_MAX+1];

   ncmd = u_splitCommand(U_STRING_TO_PARAM(command), argv, buffer, sizeof(buffer));

   U_INTERNAL_DUMP("ncmd = %d", ncmd)

   if (ncmd == -1) return; // NB: command not found...

   U_INTERNAL_ASSERT_RANGE(1,ncmd,U_MAX_ARGS)

   if (buffer[0]) argv[0] = pathcmd = U_SYSCALL(strdup, "%S", buffer);

   U_INTERNAL_DUMP("pathcmd = %S", pathcmd)

   // NB: allocazione e copia lista argomenti

   argv_exec = U_MALLOC_VECTOR(1+ncmd+1 + U_ADD_ARGS, char); // U_ADD_ARGS -> space for addArgument()...

   (void) u__memcpy(argv_exec, argv, (1+ncmd+1) * sizeof(char*)); // NB: copia anche null terminator...

   U_DUMP_ATTRS(argv_exec)
}

U_NO_EXPORT void UCommand::setEnvironment(const UString& env)
{
   U_TRACE(0, "UCommand::setEnvironment(%.*S)", U_STRING_TO_TRACE(env))

   freeEnvironment();

   char* argp[U_MAX_ARGS];

   env.duplicate();

   nenv = u_split(U_STRING_TO_PARAM(env), argp, 0);

   U_INTERNAL_DUMP("nenv = %d", nenv)

   U_INTERNAL_ASSERT_RANGE(1,nenv,U_MAX_ARGS)

   // NB: allocazione e copia lista argomenti

   envp = envp_exec = U_MALLOC_VECTOR(nenv+1, char); // NB: considera anche null terminator...

   (void) u__memcpy(envp_exec, argp, (nenv+1) * sizeof(char*)); // NB: copia anche null terminator...

   U_INTERNAL_DUMP("envp = %p", envp)

   U_DUMP_EXEC(argv_exec, envp)
}

void UCommand::setEnvironment(const UString* penv)
{
   U_TRACE(0, "UCommand::setEnvironment(%p)", penv)

   U_INTERNAL_ASSERT_POINTER(penv)
   U_INTERNAL_ASSERT_POINTER(argv_exec)

   environment = *penv;

   if ((flag_expand = penv->find('$')) == U_NOT_FOUND) setEnvironment(environment);

   U_INTERNAL_DUMP("flag_expand = %u", flag_expand)
}

void UCommand::setFileArgument()
{
   U_TRACE(0, "UCommand::setFileArgument()")

   U_INTERNAL_ASSERT_POINTER(argv_exec)

   U_INTERNAL_DUMP("ncmd = %d", ncmd)

   for (int32_t i = ncmd; i >= 2; --i)
      {
      U_INTERNAL_DUMP("argv_exec[%d] = %S", i, argv_exec[i])

      if (U_STREQ(argv_exec[i], "$FILE"))
         {
         nfile = i;

         break;
         }
      }

   U_INTERNAL_DUMP("nfile = %d", nfile)
}

void UCommand::setNumArgument(int32_t n, bool bfree)
{
   U_TRACE(1, "UCommand::setNumArgument(%d,%b)", n, bfree)

   U_INTERNAL_ASSERT_POINTER(argv_exec)

   // check if we need to free strndup() malloc... (see URPCGenericMethod::execute())

   if (bfree &&
       ncmd > n)
      {
      while (ncmd != n)
         {
         U_SYSCALL_VOID(free, "%p", argv_exec[ncmd]);

         --ncmd;
         }
      }

   argv_exec[(ncmd = n) + 1] = 0;
}

U_NO_EXPORT void UCommand::setStdInOutErr(int fd_stdin, bool flag_stdin, bool flag_stdout, int fd_stderr)
{
   U_TRACE(0, "UCommand::setStdInOutErr(%d,%b,%b,%d)", fd_stdin, flag_stdin, flag_stdout, fd_stderr)

   if (flag_stdin)
      {
      if (fd_stdin == -1)
         {
         UProcess::pipe(STDIN_FILENO); // UProcess::filedes[0] is for READING,
                                       // UProcess::filedes[1] is for WRITING
#     ifdef __MINGW32__
         // Ensure the write handle to the pipe for STDIN is not inherited
         (void) U_SYSCALL(SetHandleInformation, "%p,%ld,%ld", UProcess::hFile[1], HANDLE_FLAG_INHERIT, 0);
#     endif
         }
      else
         {
         UProcess::filedes[0] = fd_stdin;
         }
      }

   if (flag_stdout)
      {
      UProcess::pipe(STDOUT_FILENO);   // UProcess::filedes[2] is for READING,
                                       // UProcess::filedes[3] is for WRITING
#  ifdef __MINGW32__
      // Ensure the read handle to the pipe for STDOUT is not inherited
      (void) U_SYSCALL(SetHandleInformation, "%p,%ld,%ld", UProcess::hFile[2], HANDLE_FLAG_INHERIT, 0);
#  endif
      }

   if (fd_stderr != -1)
      {
      UProcess::filedes[5] = fd_stderr;
      }
}

U_NO_EXPORT void UCommand::execute(bool flag_stdin, bool flag_stdout, bool flag_stderr)
{
   U_TRACE(0, "UCommand::execute(%b,%b,%b)", flag_stdin, flag_stdout, flag_stderr)

   U_INTERNAL_ASSERT_POINTER(argv_exec)

   if (flag_expand != U_NOT_FOUND)
      {
      // NB: it must remain in this way (I don't understand why...)

      environment = UStringExt::expandEnvironmentVar(environment, &environment);

      setEnvironment(environment);
      }

#ifdef DEBUG
   char* _end  = (char*) command.end();
   char* begin =         command.data();

   U_INTERNAL_DUMP("begin = %p end = %p argv_exec[1] = %p %S", begin, _end, argv_exec[1], argv_exec[1])

#  ifndef __MINGW32__
   U_INTERNAL_ASSERT_RANGE(begin,argv_exec[1],_end)
#  endif

   int32_t i;

   // NB: we cannot check argv a cause of addArgument()...

   for (i = 0; argv_exec[1+i]; ++i) {}

   U_INTERNAL_ASSERT_EQUALS(i,ncmd)

   if (envp            &&
       envp != environ &&
       flag_expand == U_NOT_FOUND)
      {
      _end  = (char*) environment.end();
      begin =         environment.data();

      U_INTERNAL_DUMP("begin = %p _end = %p envp[0] = %p %S", begin, _end, envp[0], envp[0])

      for (i = 0; envp[i]; ++i)
         {
         U_INTERNAL_ASSERT_RANGE(begin,envp[i],_end)
         }

      U_INTERNAL_ASSERT_EQUALS(i,nenv)
      }
#endif

   exit_value = status = -1;
   pid        = UProcess::execute(U_PATH_CONV(argv_exec[0]), argv_exec+1, envp, flag_stdin, flag_stdout, flag_stderr);
}

U_NO_EXPORT bool UCommand::postCommand(UString* input, UString* output)
{
   U_TRACE(1, "UCommand::postCommand(%p,%p)", input, output)

   U_INTERNAL_DUMP("pid = %d", pid)

   if (input)
      {
                    UFile::close(UProcess::filedes[0]); // UProcess::filedes[0] for READING...
                                 UProcess::filedes[0] = 0;
      if (pid <= 0) UFile::close(UProcess::filedes[1]); // UProcess::filedes[1] for WRITING...
      }

   if (output)
      {
                    UFile::close(UProcess::filedes[3]); // UProcess::filedes[3] for WRITING...
                                 UProcess::filedes[3] = 0;
      if (pid <= 0) UFile::close(UProcess::filedes[2]); // UProcess::filedes[2] for READING...
      }

   exit_value = status = -1;

   if (pid <= 0)
      {
      UProcess::filedes[1] = UProcess::filedes[2] = 0;

      U_RETURN(false);
      }

   if (input &&
       input != (void*)-1) // special value...
      {
      U_ASSERT(input->empty() == false)

      (void) UNotifier::write(UProcess::filedes[1], input->data(), input->size());

      UFile::close(UProcess::filedes[1]);
                   UProcess::filedes[1] = 0;
      }

   exit_value = status = 0;

   if (output &&
       output != (void*)-1) // special value...
      {
      output->setBuffer(U_CAPACITY); // to avoid reserve()...

      bool kill_command = (UNotifier::waitForRead(UProcess::filedes[2], timeoutMS) <= 0);

#  ifdef __MINGW32__
      // NB: we don't have select() on pipe...
      if (kill_command && timeoutMS == -1) kill_command = false;
#  endif

      if (kill_command == false) UServices::readEOF(UProcess::filedes[2], *output);

      UFile::close(UProcess::filedes[2]);
                   UProcess::filedes[2] = 0;

      if (kill_command)
         {
         // Timeout execeded

         UProcess::kill(pid, SIGTERM);

         UTimeVal(1L).nanosleep();

         UProcess::kill(pid, SIGKILL);
         }

      bool result = wait();

      if (kill_command)
         {
         exit_value = -EAGAIN;

         U_RETURN(false);
         }

      U_RETURN(result);
      }

   U_RETURN(true);
}

U_NO_EXPORT bool UCommand::wait()
{
   U_TRACE(0, "UCommand::wait()")

   UProcess::waitpid(pid, &status, 0);

   exit_value = UProcess::exitValue(status);

   U_RETURN(exit_value == 0);
}

void UCommand::setMsgError(const char* cmd)
{
   U_TRACE(0, "UCommand::setMsgError(%S)", cmd)

   U_INTERNAL_ASSERT_EQUALS(u_buffer_len, 0)

   U_INTERNAL_DUMP("pid = %d exit_value = %d status = %d", pid, exit_value, status)

   // NB: il carattere '<' e' riservato per xml...

   if (isStarted() == false)
      {
      u_buffer_len = u__snprintf(u_buffer, sizeof(u_buffer), "command %S didn't start %R",  cmd, 0); // NB: the last argument (0) is necessary...
      }
   else if (isTimeout())
      {
      u_buffer_len = u__snprintf(u_buffer, sizeof(u_buffer), "command %S (pid %u) excedeed time (%d secs) for execution", cmd, pid, timeoutMS / 1000);
      }
   else
      {
      u_buffer_len = u__snprintf(u_buffer, sizeof(u_buffer), "command %S started (pid %u) and ended with status: %d (%d, %s)",
                                                              cmd, pid, status, exit_value, UProcess::exitInfo(status));
      }
}

// public method...

bool UCommand::executeWithFileArgument(UString* output, UFile* file)
{
   U_TRACE(0, "UCommand::executeWithFileArgument(%p,%p)", output, file)

   int fd_stdin = -1;

   if (getNumFileArgument() == -1)
      {
      if (file->open()) fd_stdin = file->getFd();
      }
   else
      {
      setFileArgument(file->getPathRelativ());
      }

   bool result = execute(0, output, fd_stdin, -1);

   if (result == false) printMsgError();

   if (file->isOpen()) file->close();

   U_RETURN(result);
}

bool UCommand::executeAndWait(UString* input, int fd_stdin, int fd_stderr)
{
   U_TRACE(0, "UCommand::executeAndWait(%p,%d,%d)", input, fd_stdin, fd_stderr)

   bool result = execute(input, 0, fd_stdin, fd_stderr) && wait();

   U_RETURN(result);
}

bool UCommand::execute(UString* input, UString* output, int fd_stdin, int fd_stderr)
{
   U_TRACE(0, "UCommand::execute(%p,%p,%d,%d)", input, output, fd_stdin, fd_stderr)

   bool flag_stdin  = (input ? true : fd_stdin != -1),
        flag_stdout = (output != 0),
        flag_stderr = (fd_stderr != -1);

   setStdInOutErr(fd_stdin, flag_stdin, flag_stdout, fd_stderr);

   execute(flag_stdin, flag_stdout, flag_stderr);

   bool result = postCommand(input, output);

   U_RETURN(result);
}

// Return output of command

void UCommand::outputCommandWithDialog(const UString& cmd, char** penv, UString* output, int fd_stdin, int fd_stderr, bool dialog)
{
   U_TRACE(1, "UCommand::outputCommandWithDialog(%.*S,%p,%p,%d,%d,%b)", U_STRING_TO_TRACE(cmd), penv, output, fd_stdin, fd_stderr, dialog)

   U_INTERNAL_ASSERT_POINTER(output)

   UCommand tmp(cmd, penv);
   bool flag_stdin = (fd_stdin  != -1), flag_stdout, flag_stderr;

   if (dialog)
      {
      // we must have already called UProcess::pipe(STDOUT_FILENO)...

      U_INTERNAL_ASSERT_MAJOR(UProcess::filedes[2],STDOUT_FILENO)
      U_INTERNAL_ASSERT_MAJOR(UProcess::filedes[3],STDOUT_FILENO)

      flag_stdout = false;
      flag_stderr = false;
      }
   else
      {
      flag_stdout = true;
      flag_stderr = (fd_stderr != -1);
      }

   setStdInOutErr(fd_stdin, flag_stdin, flag_stdout, fd_stderr);

   tmp.execute(flag_stdin, flag_stdout, flag_stderr);

   if (postCommand(0, output) == false || exit_value) setMsgError(cmd.data());
}

UString UCommand::outputCommand(const UString& cmd, char** penv, int fd_stdin, int fd_stderr)
{
   U_TRACE(0, "UCommand::outputCommand(%.*S,%p,%d,%d)", U_STRING_TO_TRACE(cmd), penv, fd_stdin, fd_stderr)

   UString output(U_CAPACITY);

   outputCommandWithDialog(cmd, penv, &output, fd_stdin, fd_stderr, false);

   if (exit_value) printMsgError();

   U_RETURN_STRING(output);
}

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UCommand::dump(bool _reset) const
{
   *UObjectIO::os << "pid                     " << pid                  << '\n'
                  << "ncmd                    " << ncmd                 << '\n'
                  << "nfile                   " << nfile                << '\n'
                  << "exit_value              " << exit_value           << '\n'
                  << "command     (UString    " << (void*)&command      << ")\n"
                  << "environment (UString    " << (void*)&environment  << ')';

   if (_reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
