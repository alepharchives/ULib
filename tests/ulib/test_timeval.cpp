// test_timeval.cpp

#include <ulib/timeval.h>
#include <ulib/utility/interrupt.h>

static struct itimerval timeval = { { 0, 200000 }, { 0, 200000 } };

static RETSIGTYPE manage_alarm(int signo)
{
   U_TRACE(4, "[SIGALRM] manage_alarm(%d)", signo)

   (void) U_SYSCALL(setitimer,"%d,%p,%p",ITIMER_REAL,&timeval,0);
}

static void set_alarm()
{
   U_TRACE(4, "set_alarm()")

   UInterrupt::setHandlerForSignal(SIGALRM, (sighandler_t)&manage_alarm);

   (void) U_SYSCALL(setitimer,"%d,%p,%p",ITIMER_REAL,&timeval,0);
}

int U_EXPORT main (int argc, char* argv[])
{
   U_ULIB_INIT(argv);

   U_TRACE(5,"main(%d)",argc)

   UTimeVal x(0L);
   U_ASSERT(x.isZero());

   UTimeVal a(1L);
   UTimeVal y(a);

   a.add(8L, 1999999L);
   a.sub(8L, 1999999L);

   U_ASSERT(y == a);
   U_ASSERT(y == UTimeVal(1L, 0));
   U_ASSERT(y <= UTimeVal(1L, 0));
   U_ASSERT(y >= UTimeVal(1L, 0));
   U_ASSERT(y >  UTimeVal(0L, 999999));
   U_ASSERT(y <  UTimeVal(1L, 1));

   y = a;
   U_ASSERT(y == a);

   UTimeVal tv1(0);
   UTimeVal tv2(2);
   UTimeVal tv3(100);
   UTimeVal tv4(1, 100000);
   UTimeVal tv5(2);
   UTimeVal tv6(1, -100000);

   U_ASSERT(tv1 == UTimeVal(0));
   U_ASSERT(tv2 < tv3);
   U_ASSERT(tv2 <= tv2);
   U_ASSERT(tv2 >= tv4);
   U_ASSERT(tv5 >= tv6);
   U_ASSERT(tv5 != tv4);
   U_ASSERT(tv2 != tv4);
   U_ASSERT(tv1 != tv2);
   U_ASSERT(tv6 != tv1);

   U_ASSERT(y < UTimeVal(u_now));

   set_alarm();

   y.nanosleep();

   y += UTimeVal(u_now);

   U_ASSERT(y > UTimeVal(u_now));
}
