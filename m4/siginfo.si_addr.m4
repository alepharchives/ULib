dnl Define HAVE_MEMBER_SI_ADDR if `struct siginfo' have member <si_addr>

AC_DEFUN([AC_MEMBER_SI_ADDR],
[ AC_CHECK_HEADERS(signal.h)
  AC_CACHE_CHECK([for struct siginfo  have member <si_addr>], ac_cv_sys_have_member_si_addr,
    [AC_TRY_COMPILE(
      [
# include <signal.h>
      ],
      [static struct siginfo x; x.si_addr = 0;],
      ac_cv_sys_have_member_si_addr=yes,
      ac_cv_sys_have_member_si_addr=no)
    ])

  if test $ac_cv_sys_have_member_si_addr = yes; then
    AC_DEFINE_UNQUOTED(HAVE_MEMBER_SI_ADDR, 1,
[Define if struct siginfo have member <si_addr>. ])
  fi
])
