// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    thread.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/thread.h>

#include <time.h>

typedef void* (*exec_t)(void*);
typedef void  (*cleanup_t)(void*);

#ifndef HAVE_NANOSLEEP
extern "C" { int nanosleep (const struct timespec* requested_time,
                                  struct timespec* remaining); }
#endif

#define CCXX_SIG_THREAD_SUSPEND SIGSTOP
#define CCXX_SIG_THREAD_RESUME  SIGCONT

UThread::UThread()
{
   U_TRACE_REGISTER_OBJECT(0, UThread, "")

   _tid           = 0;
   _cancel        = cancelInitial;
   _suspendEnable = true;

   (void) U_SYSCALL(pthread_attr_init, "%p", &_attr);
}

UThread::~UThread()
{
   U_TRACE_UNREGISTER_OBJECT(0, UThread)

   (void) U_SYSCALL(pthread_attr_destroy, "%p", &_attr);
}

sigset_t* UThread::blockedSignals(sigset_t* sig)
{
   U_TRACE(0, "UThread::blockedSignals(%p)", sig)

   sigemptyset(sig);

   sigaddset(sig, SIGINT);
   sigaddset(sig, SIGKILL);
   sigaddset(sig, SIGHUP);
   sigaddset(sig, SIGABRT);
   sigaddset(sig, SIGALRM);
   sigaddset(sig, SIGPIPE);

   return sig;
}

void UThread::yield()
{
   U_TRACE(1, "UThread::yield()")

#ifdef CCXX_SIG_THREAD_CANCEL
   sigset_t cancel, old;

   sigemptyset(&cancel);
   sigaddset(&cancel, CCXX_SIG_THREAD_CANCEL);

   if (th->_cancel != cancelDisabled &&
       th->_cancel != cancelInitial)
      {
      (void) U_SYSCALL(pthread_sigmask, "%d,%p,%p", SIG_UNBLOCK, &cancel, &old);
      }
#else
   U_SYSCALL_VOID_NO_PARAM(pthread_testcancel);
#endif

#ifdef HAVE_PTHREAD_YIELD
   (void) U_SYSCALL_NO_PARAM(pthread_yield);
#endif

#ifdef CCXX_SIG_THREAD_CANCEL
   if (th->_cancel != cancelDisabled &&
       th->_cancel != cancelInitial)
      {
      (void) U_SYSCALL(pthread_sigmask, "%d,%p,%p", SIG_SETMASK, &old, 0);
      }
#endif
}

bool UThread::isDetached() const
{
   U_TRACE(1, "UThread::close()")

   if (_tid)
      {
      int state;

      (void) U_SYSCALL(pthread_attr_getdetachstate, "%p,%p", &_attr, &state);

      if (state == PTHREAD_CREATE_DETACHED) U_RETURN(true);
      }

   U_RETURN(false);
}

void UThread::close()
{
   U_TRACE(1, "UThread::close()")

   bool detached = isDetached();

   setCancel(cancelDisabled);

   // final can call destructor (that call Terminate)

   final();

   // see if detached, and hence self deleting

   if (detached) delete this;
}

void UThread::setSuspend(Suspend mode)
{
   U_TRACE(1, "UThread::setSuspend(%d)", mode)

   if (_tid)
      {
      _suspendEnable = (mode == suspendEnable);

#ifndef HAVE_PTHREAD_SUSPEND
#  ifdef CCXX_SIG_THREAD_SUSPEND
      sigset_t mask;

      sigemptyset(&mask);
      sigaddset(&mask, CCXX_SIG_THREAD_SUSPEND);

      switch(mode)
         {
         case suspendEnable:  (void) U_SYSCALL(pthread_sigmask, "%d,%p,%p", SIG_UNBLOCK, &mask, 0); break;
         case suspendDisable: (void) U_SYSCALL(pthread_sigmask, "%d,%p,%p", SIG_BLOCK,   &mask, 0); break;
         }
#  endif
#endif
      }
}

void UThread::suspend()
{
   U_TRACE(1, "UThread::suspend()")

   if (_tid) (void) U_SYSCALL(pthread_kill, "%d,%d", _tid, CCXX_SIG_THREAD_SUSPEND);
}

void UThread::resume()
{
   U_TRACE(1, "UThread::resume()")

   if (_tid) (void) U_SYSCALL(pthread_kill, "%d,%d", _tid, CCXX_SIG_THREAD_RESUME);
}

__noreturn void UThread::execHandler(UThread* th)
{
   U_TRACE(1, "UThread::execHandler(%p)", th)

   sigset_t mask;

   (void) U_SYSCALL(pthread_sigmask, "%d,%p,%p", SIG_BLOCK, blockedSignals(&mask), 0);

   th->_tid = U_SYSCALL_NO_PARAM(pthread_self);

   th->setSuspend(suspendEnable);

   th->yield();

   pthread_cleanup_push((cleanup_t)threadCleanup, th);

   th->initial();

   if (th->getCancel() == cancelInitial) th->setCancel(cancelDeferred);

   th->run();

   th->setCancel(cancelDisabled);

   pthread_cleanup_pop(0);

   th->close();

   U_SYSCALL_VOID(pthread_exit, "%p", 0);
}

int UThread::start()
{
   U_TRACE(1, "UThread::start()")

   int result = (_tid ? -1 : U_SYSCALL(pthread_create, "%p,%p,%p,%p", &_tid, &_attr, (exec_t)execHandler, this));

   U_RETURN(result);
}

void UThread::setCancel(int mode)
{
   U_TRACE(1, "UThread::setCancel(%d)", mode)

   int old;

   switch (mode)
      {
      case cancelImmediate:
      case cancelDeferred:
         {
         (void) U_SYSCALL(pthread_setcancelstate, "%d,%p",                           PTHREAD_CANCEL_ENABLE,        &old);
         (void) U_SYSCALL(pthread_setcanceltype,  "%d,%p", (mode == cancelDeferred ? PTHREAD_CANCEL_DEFERRED
                                                                                   : PTHREAD_CANCEL_ASYNCHRONOUS), &old);
         }
      break;

      case cancelInitial:
      case cancelDisabled:
         (void) U_SYSCALL(pthread_setcancelstate, "%d,%p", PTHREAD_CANCEL_DISABLE, &old);
      break;
      }

   _cancel = mode;
}

int UThread::enterCancel()
{
   U_TRACE(1, "UThread::enterCancel()")

    int old = _cancel;

    if (old != cancelDisabled &&
        old != cancelImmediate)
       {
       setCancel(cancelImmediate);

       U_SYSCALL_VOID_NO_PARAM(pthread_testcancel);
       }

    U_RETURN(old);
}

void UThread::exitCancel(int old)
{
   U_TRACE(1, "UThread::exitCancel(%d)", old)

   if (old != _cancel)
      {
      U_SYSCALL_VOID_NO_PARAM(pthread_testcancel);

      setCancel(old);
      }
}

void UThread::sleep(time_t timeout)
{
   U_TRACE(1, "UThread::sleep(%ld)", timeout)

   struct timespec ts;
   int old = enterCancel();

   ts.tv_sec  =  timeout / 1000;
   ts.tv_nsec = (timeout % 1000) * 1000000;

   (void) U_SYSCALL(nanosleep, "%p,%p", &ts, 0);

   exitCancel(old);
}

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UThread::dump(bool reset) const
{
   *UObjectIO::os << "_tid           " << _tid << '\n'
                  << "_cancel        " << _cancel << '\n'
                  << "_suspendEnable " << _suspendEnable;

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
