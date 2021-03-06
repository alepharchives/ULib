// ============================================================================
//
// = LIBRARY
//    ulibdbg - c++ library
//
// = FILENAME
//    crono.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIBDBG_CRONO_H
#define ULIBDBG_CRONO_H 1

#include <ulib/base/base.h>

struct U_EXPORT UCrono {

   void start() { (void) gettimeofday(&t0, 0); }

   void stop()
      {
      (void) gettimeofday(&t1, 0);

      t1.tv_sec  -= t0.tv_sec;
      t1.tv_usec -= t0.tv_usec;

      if (t1.tv_usec < 0L)
         {
         t1.tv_sec--;
         t1.tv_usec += 1000000L;
         }
      }

   double getTimeStart() const { return (double) t0.tv_sec + (t0.tv_usec / 1000000.); }
   double getTimeStop() const  { return (double) t1.tv_sec + (t1.tv_usec / 1000000.); }

   long   getTimeElapsed() const          { return (t1.tv_sec *    1000L) + (t1.tv_usec /    1000L); }
   double getTimeElapsedInSecond() const  { return (t1.tv_sec * 1000000.) + (t1.tv_usec / 1000000.); }

private:
   struct timeval t0, t1;
};

#endif
