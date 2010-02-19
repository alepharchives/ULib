dnl @synopsis AC_COMPILATION_OPTIONS
dnl Macros that add compilation options to a `configure' script.
dnl AC_COMPILATION_OPTIONS

AC_DEFUN([AC_COMPILATION_OPTIONS],[
	AC_MSG_CHECKING(if you want to enable use of memory pool)
	AC_ARG_ENABLE(memory-pool,
				[  --enable-memory-pool    enable memory pool features [[default=yes]]])
	if test -z "$enable_memory_pool" ; then
		enable_memory_pool="yes"
      AC_DEFINE(U_MEMORY_POOL, 1, [enable memory pool features])
	fi
	AC_MSG_RESULT([${enable_memory_pool}])

	AC_MSG_CHECKING(if you want to enable Large File Support)
	AC_ARG_ENABLE(LFS,
				[  --enable-LFS            enable Large File Support [[default=yes]]])
	if test -z "$enable_LFS" ; then
		enable_LFS="yes"
      AC_DEFINE(HAVE_LFS, 1, [enable Large File Support features])
	fi
	AC_MSG_RESULT([$enable_LFS])

	AC_MSG_CHECKING(if you want to enable use of precompiled headers)
	AC_ARG_ENABLE(pch,
				[  --enable-pch            enables precompiled header support (currently only gcc >= 3.4) [[default=no]]])
	if test -z "$enable_pch" ; then
		enable_pch="no"
	fi
	AC_MSG_RESULT([$enable_pch])

	AC_MSG_CHECKING(if you want to enable build of ZIP support)
	AC_ARG_ENABLE(zip,
				[  --enable-zip            enable build of ZIP support - require option --with-zlib [[default=no]]])
	if test -z "$enable_zip" ; then
		enable_zip="no"
	fi
	AC_MSG_RESULT([$enable_zip])

	AC_MSG_CHECKING(if you want to enable use of coverage)
	AC_ARG_ENABLE(coverage,
  				[  --enable-coverage       enable coverage [[default=no]]])
	if test -z "$enable_coverage" ; then
		enable_coverage="no"
	elif test "$enable_coverage" = "yes" ; then
      CXXFLAGS="${CXXFLAGS} -fprofile-arcs -ftest-coverage"
	fi
	AC_MSG_RESULT([$enable_coverage])

	AC_MSG_CHECKING(if you want to enable mode final for build of ulib library)
	AC_ARG_ENABLE(final,
				[  --enable-final          build size optimized apps (experimental - needs lots of memory) [[default=no]]])
	if test -z "$enable_final" ; then
		enable_final="no"
	fi
	AC_MSG_RESULT([$enable_final])

	dnl Check if the linker supports --enable-new-dtags and --as-needed

	AC_MSG_CHECKING(for use of the new linker flags)
	AC_ARG_ENABLE(new-ldflags,
				[  --enable-new-ldflags    enable the new linker flags (enable-new-dtags,as-needed,...) [[default=yes]]])
	if test -z "$enable_new_ldflags" ; then
		enable_new_ldflags="yes"
	fi
	AC_MSG_RESULT([${enable_new_ldflags}])

	AC_MSG_CHECKING(for use of our versions of the C++ memory operators)
	AC_ARG_ENABLE(overload-new-delete,
				[  --enable-overload-new-delete  enable use of our versions of the C++ memory operators [[default=no]]])
	if test -z "$enable_overload_new_delete" ; then
		enable_overload_new_delete="no"
	fi
	AC_MSG_RESULT([${enable_overload_new_delete}])

	dnl check for GNUC visibility support

	AC_CACHE_CHECK(whether ${CXX} supports -fvisibility-inlines-hidden,
	ac_cv_cxx_visibility_inlines_hidden_flag,
	[
	echo '#include <sstream>' >conftest.c
	echo 'void f(){}' >>conftest.c
	if test -z "`${CXX} -fvisibility-inlines-hidden -c conftest.c 2>&1`"; then
		ac_cv_cxx_visibility_inlines_hidden_flag=yes
	else
		ac_cv_cxx_visibility_inlines_hidden_flag=no
	fi
	rm -f conftest*
	])
])
