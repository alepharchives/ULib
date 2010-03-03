// test_notifier.cpp

#include <ulib/file.h>
#include <ulib/timer.h>
#include <ulib/notifier.h>

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
static int i, fd_input, fd_output;

class handlerOutput : public UEventFd {
public:

   handlerOutput()
      {
      U_TRACE_REGISTER_OBJECT(0, handlerOutput, "", 0)

      fd      = fd_output;
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

class handlerInput : public UEventFd {
public:

   handlerInput()
      {
      U_TRACE_REGISTER_OBJECT(0, handlerInput, "", 0)

      fd      = fd_input;
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
            if (UNotifier::waitForWrite(fd_output) > 0)
               {
               static handlerOutput* handler_output;

               if (handler_output == 0)
                  {
                  handler_output = U_NEW(handlerOutput);

                  UNotifier::insert(handler_output, true);
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

   int fds[2], n = (argc > 1 ? atoi(argv[1]) : 5);

   pipe(fds);

// fd_input  = U_SYSCALL(open, "%S,%d",    "inp/notifier.input",  O_RDONLY);                       // STDIN_FILENO
// fd_output = U_SYSCALL(open, "%S,%d,%d", "tmp/notifier.output", O_WRONLY | O_CREAT, PERM_FILE);  // STDOUT_FILENO

   // fds[0] is for READING, fds[1] is for WRITING

   fd_input  = fds[0];
   fd_output = fds[1];

   handlerInput* c = U_NEW(handlerInput);
   handlerInput* d = U_NEW(handlerInput);

   UNotifier::init(c);
   UNotifier::erase(c, true);
   UNotifier::insert(d, true);

#ifdef __unix__
   U_ASSERT(UNotifier::waitForRead( fds[0], 500) <= 0)
#endif
   U_ASSERT(UNotifier::waitForWrite(fds[1], 500) == 1)

   MyAlarm1* a = U_NEW(MyAlarm1(1L, 0L));
   MyAlarm2* b = U_NEW(MyAlarm2(1L, 0L));

   UTimer::init(true);
   UTimer::insert(a);
   UTimer::insert(b);

#ifdef DEBUG
   if (argc > 2) UTimer::printInfo(cout);
#endif

   UEventTime timeout;

   for (i = 0; i < n; ++i)
      {
      timeout.setMicroSecond(100L * 1000L);

      (void) U_SYSCALL(write, "%d,%p,%u", fd_output, U_CONSTANT_TO_PARAM("hello, world"));

      (void) UNotifier::waitForEvent(&timeout);

      (void) UNotifier::waitForRead(fds[0], 500);

      (void) UTimer::insert(U_NEW(MyAlarm1(1L, 0L)));

      timeout.nanosleep();

#  ifdef DEBUG
      if (argc > 2) UTimer::printInfo(cout);
#  endif
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

#ifdef __unix__
   U_ASSERT(UNotifier::waitForRead( fd_input,  1 * 1000) <= 0)
#endif
   U_ASSERT(UNotifier::waitForWrite(fd_output, 1 * 1000) == 1)
}
