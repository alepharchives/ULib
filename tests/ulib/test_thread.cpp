// test_thread.cpp

#include <ulib/thread.h>

class ThreadTest: public UThread {
public:
   volatile int n;

   ThreadTest() {}

   void run()
      {
      n = 1;

      // wait for main thread

      if (!WaitNValue(2)) return;

      // increment infinitely

      while (true)
         {
         yield();

         n = n+1;
         }
      }

   bool WaitNValue(int value)
      {
      for (int i=0;; ++i)
         {
         if (n == value) break;
         if (i >= 100) return false;

         sleep(10);
         }

      return true;
      }

   bool WaitChangeNValue(int value)
      {
      for (int i=0;; ++i)
         {
         if (n != value) break;

         if (i >= 100) return false;

         sleep(10);
         }

      return true;
      }

   bool TestChange(bool shouldChange)
      {
      if (shouldChange) printf("- thread should change n...");
      else              printf("- thread should not change n...");

      if (WaitChangeNValue(n) == shouldChange)
         {
         printf("ok\n");

         return true;
         }

      printf("ko\n");

      return false;
      }

#ifdef DEBUG
   const char* dump(bool reset) const { return UThread::dump(reset); }
#endif
};

#undef ERROR
#undef OK
#define ERROR {printf("ko\n"); return 1; }
#define OK    {printf("ok\n"); }

#define TEST_CHANGE(b) if (!test.TestChange(b)) return 1;

int U_EXPORT main(int argc, char* argv[])
{
   U_ULIB_INIT(argv);

   U_TRACE(5,"main(%d)",argc)

   ThreadTest test;

   // test only thread, without sincronization
   printf("***********************************************\n");
   printf("* Testing class Thread without syncronization *\n");
   printf("***********************************************\n");

   printf("Testing thread creation\n\n");
   test.n = 0;
   test.start();

   // wait for n == 1
   printf("- thread should set n to 1...");
   if (test.WaitNValue(1)) OK
   else ERROR;

   // increment number in thread
   printf("\nTesting thread is working\n\n");
   test.n = 2;
   TEST_CHANGE(true);
   TEST_CHANGE(true);

   // suspend thread, variable should not change
   printf("\nTesting suspend & resume\n\n");
   test.suspend();
   TEST_CHANGE(false);
   TEST_CHANGE(false);

   // resume, variable should change
   test.resume();
   TEST_CHANGE(true);
   TEST_CHANGE(true);

   printf("\nTesting recursive suspend & resume\n\n");
   test.suspend();
   test.suspend();
   TEST_CHANGE(false);
   TEST_CHANGE(false);

   test.resume();
   TEST_CHANGE(false);
   TEST_CHANGE(false);
   test.resume();
   TEST_CHANGE(true);
   TEST_CHANGE(true);

   printf("\nTesting no suspend on resume\n\n");
   test.resume();
   TEST_CHANGE(true);
   TEST_CHANGE(true);

   // suspend thread, variable should not change
   printf("\nTesting resuspend\n\n");
   test.suspend();
   TEST_CHANGE(false);
   TEST_CHANGE(false);

   printf("\nNow program should finish... :)\n");
   test.resume();

   return 0;
}
