####################################################################################
# ULib.m4 - Example autoconf macro showing how to find ULib library and header files
####################################################################################

dnl @synopsis AC_ULIB
dnl 
dnl This macro tries to find the ULib library and header files.
dnl
dnl We define the following configure script flags:
dnl
dnl		--with-ULib: Give prefix for both library and headers, and try
dnl			to guess subdirectory names for each.
dnl		--with-ULib-lib: Similar to --with-ULib, but for library only.
dnl		--with-ULib-include: Similar to --with-ULib, but for headers only.

AC_DEFUN([AC_ULIB],
[
AC_CACHE_CHECK([for ULib library stuff], ac_cv_ULib,
[
	#
	# Set up configure script macros
	#
	AC_ARG_WITH(ULib,
		[  --with-ULib=<path>     path containing ULib header and library subdirs],
		[ULIB_lib_check="$with_ULib/lib $with_ULib/lib/ULib"
		  ULIB_inc_check="$with_ULib/include $with_ULib/include/ULib"],
		[ULIB_lib_check="/usr/local/ULib/lib /usr/local/lib/ULib /opt/ULib/lib /usr/lib/ULib /usr/local/lib /usr/lib"
		  ULIB_inc_check="/usr/local/ULib/include /usr/local/include/ULib /opt/ULib/include /usr/local/include/ULib /usr/local/include /usr/include/ULib /usr/include"])
	AC_ARG_WITH(ULib-lib,
		[  --with-ULib-lib=<path> directory path of ULib library],
		[ULIB_lib_check="$with_ULib_lib $with_ULib_lib/lib $with_ULib_lib/lib/ULib"])
	AC_ARG_WITH(ULib-include,
		[  --with-ULib-include=<path> directory path of ULib headers],
		[ULIB_inc_check="$with_ULib_include $with_ULib_include/include $with_ULib_include/include/ULib"])

	#
	# Look for ULib library
	#
	ULIB_libdir=
	for dir in $ULIB_lib_check
	do
		if test -d "$dir" && \
			( test -f "$dir/libulib.so" ||
			  test -f "$dir/libulib.a" )
		then
			ULIB_libdir=$dir
			break
		fi
	done

	if test -z "$ULIB_libdir"
	then
		AC_MSG_ERROR([Didn't find the ULib library dir in '$ULIB_lib_check'])
	fi

	case "$ULIB_libdir" in
		/* ) ;;
		* )  AC_MSG_ERROR([The ULib library directory ($ULIB_libdir) must be an absolute path.]) ;;
	esac

	AC_MSG_RESULT([lib in $ULIB_libdir])

	case "$ULIB_libdir" in
	  /usr/lib) ;;
	  *) LDFLAGS="$LDFLAGS -L${ULIB_libdir}" ;;
	esac


	#
	# Look for ULib headers
	#
	AC_MSG_CHECKING([for ULib header directory])
	ULIB_incdir=
	for dir in $ULIB_inc_check
	do
		if test -d "$dir" && test -f "$dir/all.h"
		then
			ULIB_incdir=$dir
			break
		fi
	done

	if test -z "$ULIB_incdir"
	then
		AC_MSG_ERROR([Didn't find the ULib header dir in '$ULIB_inc_check'])
	fi

	case "$ULIB_incdir" in
		/* ) ;;
		* )  AC_MSG_ERROR([The ULib header directory ($ULIB_incdir) must be an absolute path.]) ;;
	esac

	AC_MSG_RESULT([$ULIB_incdir])

	CPPFLAGS="$CPPFLAGS -I${ULIB_incdir}"

	AC_MSG_CHECKING([that we can build ULib programs])
	AC_COMPILE_IFELSE(
		[AC_LANG_PROGRAM([#include <all.h>],
		[UString s; s.c_str();])],
		ac_cv_ULib=yes,
		AC_MSG_ERROR(no))
])]) dnl End ULIB

