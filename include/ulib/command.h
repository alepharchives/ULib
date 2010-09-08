// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    command.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_COMMAND_H
#define ULIB_COMMAND_H 1

#include <ulib/string.h>

#include <errno.h>

class UFile;
class UDialog;
class UServer_Base;
class UProxyPlugIn;

#define U_ADD_ARGS  100 
#define U_MAX_ARGS 8192 

class U_EXPORT UCommand {
public:

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   // COSTRUTTORI

   void zero()
      {
      envp = 0;
      pathcmd = 0;
      ncmd = nenv = nfile = 0;
      argv_exec = envp_exec = 0;
      }

   void reset()
      {
      U_TRACE(0, "UCommand::reset()")

      freeCommand();
      freeEnvironment();

      zero();
      }

   UCommand()
      {
      U_TRACE_REGISTER_OBJECT(0, UCommand, "", 0)

      zero();
      }

   UCommand(const UString& cmd, char** penv) : command(cmd)
      {
      U_TRACE_REGISTER_OBJECT(0, UCommand, "%.*S,%p", U_STRING_TO_TRACE(cmd), penv)

      zero();
      setCommand();
      setEnvironment(penv);
      }

   UCommand(const UString& cmd, const UString* penv = 0) : command(cmd)
      {
      U_TRACE_REGISTER_OBJECT(0, UCommand, "%.*S,%p", U_STRING_TO_TRACE(cmd), penv)

      zero();
      setCommand();
      setEnvironment(penv);
      }

   ~UCommand()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UCommand)

      reset();
      }

   // MANAGE GENERIC ENVIRONMENT

   void setEnvironment(char** penv)
      {
      U_TRACE(0, "UCommand::setEnvironment(%p)", penv)

      setEnvironment((const UString*)0);

      envp = penv;
      }

   void setEnvironment(const UString* env);

   // MANAGE GENERIC ARGUMENT

   void delArgument()
      {
      U_TRACE(0, "UCommand::delArgument()")

      U_INTERNAL_ASSERT_POINTER(argv_exec)

      argv_exec[ncmd--] = 0;

      U_INTERNAL_DUMP("ncmd = %d", ncmd)

      U_INTERNAL_ASSERT_RANGE(1,ncmd,U_ADD_ARGS)
      }

   void addArgument(const char* argument)
      {
      U_TRACE(0, "UCommand::addArgument(%S)", argument)

      U_INTERNAL_ASSERT_POINTER(argv_exec)

      argv_exec[++ncmd] = (char*) argument;
      argv_exec[ncmd+1] = 0;

      U_INTERNAL_DUMP("ncmd = %d", ncmd)

      U_INTERNAL_ASSERT_RANGE(1,ncmd,U_ADD_ARGS)
      }

   void setArgument(int n, const char* argument)
      {
      U_TRACE(0, "UCommand::setArgument(%d,%S)", n, argument)

      U_INTERNAL_ASSERT_RANGE(2,n,ncmd)
      U_INTERNAL_ASSERT_POINTER(argv_exec)

      argv_exec[n] = (char*) argument;
      }

   void setLastArgument(const char* argument)
      {
      U_TRACE(0, "UCommand::setLastArgument(%S)", argument)

      U_INTERNAL_ASSERT_POINTER(argv_exec)
      U_INTERNAL_ASSERT_EQUALS(argv_exec[ncmd+1],0)

      argv_exec[ncmd] = (char*) argument;
      }

   void setNumArgument(int32_t n = 1, bool bfree = false);

   // MANAGE FILE ARGUMENT

   void setFileArgument();
   void setFileArgument(const char* pathfile)
      {
      U_TRACE(0, "UCommand::setFileArgument(%S)", pathfile)

      U_INTERNAL_ASSERT_POINTER(argv_exec)

      U_INTERNAL_DUMP("ncmd = %d", ncmd)

      U_INTERNAL_ASSERT_RANGE(2,nfile,ncmd)

      argv_exec[nfile] = (char*) pathfile;
      }

   // VARIE

   int32_t getNumArgument() const       { return ncmd; }
   int32_t getNumFileArgument() const   { return nfile; }

   UString getStringCommand()           { return command; }
   UString getStringEnvironment()       { return environment; }

   void setCommand(const UString& cmd)
      {
      U_TRACE(0, "UCommand::setCommand(%.*S)", U_STRING_TO_TRACE(cmd))

      command = cmd;

      setCommand();
      }

   void set(const UString& cmd, char** penv)
      {
      U_TRACE(0, "UCommand::set(%.*S,%p)", U_STRING_TO_TRACE(cmd), penv)

      setCommand(cmd);
      setEnvironment(penv);
      }

   void set(const UString& cmd, const UString* penv)
      {
      U_TRACE(0, "UCommand::set(%.*S,%p)", U_STRING_TO_TRACE(cmd), penv)

      setCommand(cmd);
      setEnvironment(penv);
      }

   bool isShellScript() const
      {
      U_TRACE(0, "UCommand::isShellScript()")

      U_ASSERT(command.empty() == false)

      bool result = U_STRNEQ(command.data(), U_PATH_SHELL);

      U_RETURN(result);
      }

   char* getCommand() const
      {
      U_TRACE(0, "UCommand::getCommand()")

      char* result = argv_exec[(isShellScript() ? 2 : 0)];

      U_RETURN(result);
      }

   // SERVICES

   static pid_t pid;
   static int timeoutMS; // specified the timeout value, in milliseconds for read output. A negative value indicates no timeout, i.e. an infinite wait.
   static int exit_value;

   static void setTimeout(int seconds) { timeoutMS = (seconds * 1000); }

   // run command

          bool    executeWithFileArgument(UString* output, UFile* file);

          bool    executeAndWait(UString* input = 0,                      int fd_stdin = -1, int fd_stderr = -1);
          bool    execute(       UString* input = 0, UString* output = 0, int fd_stdin = -1, int fd_stderr = -1);

   static UString outputCommand(UString& cmd, char** envp = 0,            int fd_stdin = -1, int fd_stderr = -1);

   bool checkForExecute(int mode = R_OK | X_OK)
      {
      U_TRACE(1, "UCommand::checkForExecute(%d)", mode)

      U_INTERNAL_ASSERT_POINTER(argv_exec)

      bool result = (U_SYSCALL(access, "%S,%d", argv_exec[0], mode) == 0);

      U_RETURN(result);
      }

   // MANAGE MESSAGE ERROR

   static bool isStarted()
      {
      U_TRACE(0, "UCommand::isStarted()")

      bool result = (pid > 0);

      U_RETURN(result);
      }

   static bool isTimeout()
      {
      U_TRACE(0, "UCommand::isTimeout()")

      bool result = (exit_value == -EAGAIN);

      U_RETURN(result);
      }

   static void printMsgError()
      {
      U_TRACE(0, "UCommand::printMsgError()")

      U_INTERNAL_ASSERT_MAJOR(u_buffer_len, 0)

      U_WARNING("%.*s", u_buffer_len, u_buffer);

      u_buffer_len = 0;
      }

   static void setMsgError(const char* cmd);

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   char** envp;
   char* pathcmd;
   char** argv_exec;
   char** envp_exec;
   uint32_t flag_expand;
   int32_t ncmd, nenv, nfile;
   UString command, environment;

   void setCommand();
   void freeCommand();
   void freeEnvironment();

   void reset(const UString& cmd, const UString& env)
      {
      U_TRACE(0, "UCommand::reset(%.*S,%.*S)", U_STRING_TO_TRACE(cmd), U_STRING_TO_TRACE(env))

      command     = cmd;
      environment = env;
      }

   static void outputCommand(UString& cmd, char** envp, UString* output, int fd_stdin, int fd_stderr, bool dialog);

private:
                 void setEnvironment(const UString& env) U_NO_EXPORT;
          inline void execute(bool flag_stdin, bool flag_stdout, bool flag_stderr) U_NO_EXPORT;

   static        bool wait() U_NO_EXPORT;
   static        bool postCommand(UString* input, UString* output) U_NO_EXPORT;
   static        void setStdInOutErr(int fd_stdin, bool flag_stdin, bool flag_stdout, int fd_stderr) U_NO_EXPORT;

   UCommand(const UCommand&)            {}
   UCommand& operator=(const UCommand&) { return *this; }

   friend class UDialog;
   friend class UServer_Base;
   friend class UProxyPlugIn;
};

#endif
