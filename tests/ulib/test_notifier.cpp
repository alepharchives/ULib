// test_notifier.cpp

#include <ulib/notifier.h>
#include <ulib/timer.h>

#include <fcntl.h>
#include <iostream>

class MyAlarm1 : public UEventTime {
public:

   // COSTRUTTORI

   MyAlarm1(long sec, long usec) : UEventTime(sec, usec)
      {
      U_TRACE_REGISTER_OBJECT(0, MyAlarm1, "%ld,%ld", sec, usec)
      }

   ~MyAlarm1()
      {
      U_TRACE_UNREGISTER_OBJECT(0, MyAlarm1)
      }

   virtual int handlerTime()
      {
      U_TRACE(0, "MyAlarm1::handlerTime()")

      // return value:
      // ---------------
      // -1 - normal
      //  0 - monitoring
      // ---------------

      U_RETURN(-1);
      }

#ifdef DEBUG
   const char* dump(bool reset) const { return UEventTime::dump(reset); }
#endif
};

class MyAlarm2 : public MyAlarm1 {
public:

   // COSTRUTTORI

   MyAlarm2(long sec, long usec) : MyAlarm1(sec, usec)
      {
      U_TRACE_REGISTER_OBJECT(0, MyAlarm2, "%ld,%ld", sec, usec)
      }

   ~MyAlarm2()
      {
      U_TRACE_UNREGISTER_OBJECT(0, MyAlarm2)
      }

   virtual int handlerTime()
      {
      U_TRACE(0, "MyAlarm2::handlerTime()")

      // return value:
      // ---------------
      // -1 - normal
      //  0 - monitoring
      // ---------------

      U_RETURN(0);
      }

#ifdef DEBUG
   const char* dump(bool reset) const { return MyAlarm1::dump(reset); }
#endif
};

static char message[4096];

class handlerOutput : public UEventFd {
public:

   handlerOutput()
      {
      U_TRACE_REGISTER_OBJECT(0, handlerOutput, "", 0)

      fd      = STDOUT_FILENO;
      op_mask = U_WRITE_OUT;
      }

   ~handlerOutput()
      {
      U_TRACE_UNREGISTER_OBJECT(0, handlerOutput)
      }

   int handlerWrite()
      {
      U_TRACE(0, "handlerOutput::handlerWrite()")

      cout << "receive message: " << message << endl;

      U_RETURN(0);
      }

#ifdef DEBUG
   const char* dump(bool reset) const
      {
      *UObjectIO::os << "fd      " << fd << "\n"
                     << "op_mask " << op_mask;

      if (reset)
         {
         UObjectIO::output();

         return UObjectIO::buffer_output;
         }

      return 0;
      }
#endif
};

static int i;

class handlerInput : public UEventFd {
public:

   handlerInput()
      {
      U_TRACE_REGISTER_OBJECT(0, handlerInput, "", 0)

      fd      = U_SYSCALL(open, "%S,%d", "inp/notifier.input", O_RDONLY); // STDIN_FILENO;
      op_mask = U_READ_IN;
      }

   ~handlerInput()
      {
      U_TRACE_UNREGISTER_OBJECT(0, handlerInput)
      }

   int handlerRead()
      {
      U_TRACE(0, "handlerInput::handlerRead()")

      int bytes_read = U_SYSCALL(read, "%d,%p,%u", fd, message, 13);

      if (bytes_read > 0)
         {
         if (i & 1)
            {
            if (UNotifier::waitForWrite(STDOUT_FILENO) > 0)
               {
               static handlerOutput* handler_output;

               if (handler_output == 0)
                  {
                  handler_output = U_NEW(handlerOutput);

                  UNotifier::insert(handler_output);
                  }
               }
            }
         }

      U_RETURN(0);
      }

#ifdef DEBUG
   const char* dump(bool reset) const
      {
      *UObjectIO::os << "fd      " << fd << "\n"
                     << "op_mask " << op_mask;

      if (reset)
         {
         UObjectIO::output();

         return UObjectIO::buffer_output;
         }

      return 0;
      }
#endif
};

int U_EXPORT main(int argc, char* argv[])
{
   U_ULIB_INIT(argv);

   U_TRACE(5,"main(%d)",argc)

   MyAlarm1* a = U_NEW(MyAlarm1(1L, 0L));
   MyAlarm2* b = U_NEW(MyAlarm2(1L, 0L));

   UTimer::init(true);
   UTimer::insert(a);
   UTimer::insert(b);

#ifdef DEBUG
   if (argc > 2) UTimer::printInfo(cout);
#endif

   handlerInput* c = U_NEW(handlerInput);
   handlerInput* d = U_NEW(handlerInput);

   UNotifier::insert(c);
   UNotifier::erase(c, true);
   UNotifier::insert(d);

   int fd[2], n = (argc > 1 ? atoi(argv[1]) : 5);

   pipe(fd);

#ifdef __unix__
   U_ASSERT(UNotifier::waitForRead( fd[0], 500) <= 0)
#endif
   U_ASSERT(UNotifier::waitForWrite(fd[1], 500) == 1)

   UEventTime timeout;

   for (i = 0; i < n; ++i)
      {
      timeout.setMicroSecond(100L * 1000L);

      (void) UNotifier::waitForEvent(&timeout);

      (void) UNotifier::waitForRead(fd[0], 500);

      (void) UTimer::insert(U_NEW(MyAlarm1(1L, 0L)));

      timeout.nanosleep();

#ifdef DEBUG
      if (argc > 2) UTimer::printInfo(cout);
#endif
      }

   UTimer::stop();

#ifdef DEBUG
   if (argc > 2) UTimer::printInfo(cout);
#endif
   UTimer::clear(false);
#ifdef DEBUG
   if (argc > 2) UTimer::printInfo(cout);
#endif

#ifdef DEBUG
   if (argc > 2) UNotifier::printInfo(cout);
#endif
   UNotifier::clear();
#ifdef DEBUG
   if (argc > 2) UNotifier::printInfo(cout);
#endif

#if defined(__unix__) && !defined(HAVE_EPOLL_WAIT)
   U_ASSERT(UNotifier::waitForRead(  STDIN_FILENO, 1 * 1000) <= 0)
#endif
   U_ASSERT(UNotifier::waitForWrite(STDOUT_FILENO, 1 * 1000) == 1)
}
