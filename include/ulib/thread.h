// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    thread.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_THREAD_H
#define ULIB_THREAD_H

#include <ulib/internal/common.h>

#include <pthread.h>

class U_EXPORT UThread {
public:
   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   enum Cancel {
      cancelInitial,    /* used internally, do not use */
      cancelDeferred,   /* exit thread on cancellation pointsuch as yield */
      cancelImmediate,  /* exit befor cancellation */
      cancelDisabled    /* ignore cancellation */
   };

   enum Suspend {
      suspendEnable,    /* suspend enabled */
      suspendDisable    /* suspend disabled, Suspend do nothing */
   };

   // COSTRUTTORI

            UThread();
   virtual ~UThread();

   /**
    * All threads execute by deriving the run method of UThread.
    * This method is called after initial to begin normal operation
    * of the thread. If the method terminates, then the thread will
    * also terminate.
    */

   virtual void run() = 0;

   /**
    * The initial method is called by a newly created thread when it
    * starts execution.  This method is ran with deferred cancellation
    * disabled by default.  The Initial method is given a separate
    * handler so that it can create temporary objects on it's own
    * stack frame, rather than having objects created on run() that
    * are only needed by startup and yet continue to consume stack space.
    */

   virtual void initial() {}

    /**
     * A thread that is self terminating, either by invoking exit() or
     * leaving it's run(), will have this method called. It can be used
     * to self delete the current object assuming the object was created
     * with new on the heap rather than stack local, hence one may often
     * see final defined as "delete this" in a derived thread class.  A
     * final method, while running, cannot be terminated or cancelled by
     * another thread. Final is called for all cancellation type (even
     * immediate).
     */

   virtual void final() {}

   /**
    * When a new thread is created, it does not begin immediate
    * execution. This is because the derived class virtual tables
    * are not properly loaded at the time the C++ object is created
    * within the constructor itself, at least in some compiler/system
    * combinations. It can be started directly after the constructor
    * completes by calling the start() method.
    *
    * @return error code if execution fails.
    */

   int start();

   /**
    * Yields the current thread's CPU time slice to allow another thread to
    * begin immediate execution.
    */

   void yield();

   // SERVICES

   void sleep(time_t timeout);

   void setCancel(int mode);

   /**
    * Used to retrieve the cancellation mode in effect for the
    * selected thread.
    *
    * @return cancellation mode constant.
    */

    int getCancel() { return _cancel; }

   /**
    * This is used to help build wrapper functions in libraries
    * around system calls that should behave as cancellation
    * points but don't.
    *
    * @return saved cancel type.
    */

   int enterCancel();

   /**
    * This is used to restore a cancel block.
    *
    * @param cancel type that was saved.
    */

   void exitCancel(int cancel);

   /**
    * Suspends execution of the selected thread. Pthreads do not
    * normally support suspendable threads, so the behavior is
    * simulated with signals. On systems such as Linux that
    * define threads as processes, SIGSTOP and SIGCONT may be used.
    */

   void suspend();

   /**
    * Resumes execution of the selected thread.
    */

   void resume();

   /**
    * Check if this thread is detached.
    *
    * @return true if the thread is detached.
    */

   bool isDetached() const;

   /**
    * Sets the thread's ability to be suspended from execution. The
    * thread may either have suspend enabled (suspendEnable) or
    * disabled (suspendDisable).
    *
    * @param mode for suspend.
    */

   void setSuspend(Suspend mode);

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   int _cancel;
   pthread_t _tid;
   pthread_attr_t _attr;
   volatile bool _suspendEnable:1;

   void close(); // close current thread, free all

   static void execHandler(UThread* th) __noreturn;
   static void threadCleanup(UThread* th) { th->close(); }

   static sigset_t* blockedSignals(sigset_t* sig);

private:
   UThread(const UThread&)            {}
   UThread& operator=(const UThread&) { return *this; }
};

#endif
