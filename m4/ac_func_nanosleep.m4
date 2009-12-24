dnl If the compiler recognizes nanosleep() function, define HAVE_NANOSLEEP.

AC_DEFUN([AC_FUNC_NANOSLEEP],
[AC_CACHE_CHECK(whether the compiler recognizes nanosleep function,
ac_cv_func_nanosleep,
[AC_TRY_COMPILE([
#include <time.h>
],[nanosleep(0,0);],
 ac_cv_func_nanosleep=yes, ac_cv_func_nanosleep=no)
])
if test "$ac_cv_func_nanosleep" = yes; then
  AC_DEFINE(HAVE_NANOSLEEP,1,[define if have function nanosleep])
fi
])
