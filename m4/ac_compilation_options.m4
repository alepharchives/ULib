dnl @synopsis AC_COMPILATION_OPTIONS
dnl Macros that add compilation options to a `configure' script.
dnl AC_COMPILATION_OPTIONS

AC_DEFUN([AC_COMPILATION_OPTIONS],[
	AC_MSG_CHECKING(if you want to enable use of memory pool)
	AC_ARG_ENABLE(memory-pool,
				[  --enable-memory-pool    enable memory pool features [[default=yes]]])
	if test -z "$enable_memory_pool" ; then
		enable_memory_pool="yes"
      AC_DEFINE( ENABLE_MEMPOOL, 1, [ enable memory pool features])
	else
      AC_DEFINE(DISABLE_MEMPOOL, 1, [disable memory pool features])
	fi
	AC_MSG_RESULT([${enable_memory_pool}])

	AC_MSG_CHECKING(if you want to enable Large File Support)
	AC_ARG_ENABLE(LFS,
				[  --enable-LFS            enable Large File Support [[default=yes]]])
	if test -z "$enable_LFS" ; then
		enable_LFS="yes"
      AC_DEFINE( ENABLE_LFS, 1, [ enable Large File Support features])
	else
      AC_DEFINE(DISABLE_LFS, 1, [disable Large File Support features])
	fi
	AC_MSG_RESULT([$enable_LFS])

	AC_MSG_CHECKING(if you want to enable HTTP inotify support)
	AC_ARG_ENABLE(HIS,
				[  --enable-HIS            enable HTTP Inotify Support [[default=no]]])
	if test -z "$enable_HIS" ; then
		enable_HIS="no"
	fi
	if test "$enable_HIS" = "yes"; then
      AC_DEFINE(U_HTTP_INOTIFY_SUPPORT, 1, [enable HTTP inotify support])
	fi
	AC_MSG_RESULT([$enable_HIS])

	AC_MSG_CHECKING(if you want to enable HTTP cache request support)
	AC_ARG_ENABLE(HCRS,
				[  --enable-HCRS           enable HTTP Cache Request Support [[default=yes]]])
	if test -z "$enable_HCRS" ; then
		enable_HCRS="yes"
	fi
	if test "$enable_HCRS" != "yes"; then
      AC_DEFINE(U_HTTP_CACHE_REQUEST_DISABLE, 1, [disable HTTP cache request support])
	fi
	AC_MSG_RESULT([$enable_HCRS])

	AC_MSG_CHECKING(if you want to enable HTTP upload progress support)
	AC_ARG_ENABLE(HUPS,
				[  --enable-HUPS           enable HTTP Upload Progress Support [[default=no]]])
	if test -z "$enable_HUPS" ; then
		enable_HUPS="no"
	fi
	if test "$enable_HUPS" = "yes"; then
      AC_DEFINE(U_HTTP_UPLOAD_PROGRESS_SUPPORT, 1, [enable HTTP upload progress support])
	fi
	AC_MSG_RESULT([$enable_HUPS])

	AC_MSG_CHECKING(if you want to enable client response partial write support)
	AC_ARG_ENABLE(CRPWS,
				[  --enable-CRPWS          enable Client Response Partial Write Support [[default=no]]])
	if test -z "$enable_CRPWS" ; then
		enable_CRPWS="no"
	fi
	if test "$enable_CRPWS" = "yes"; then
      AC_DEFINE(U_CLIENT_RESPONSE_PARTIAL_WRITE_SUPPORT, 1, [enable client response partial write support])
	fi
	AC_MSG_RESULT([$enable_CRPWS])

	AC_MSG_CHECKING(if you want to enable server thread approach support)
	AC_ARG_ENABLE(thread-approach,
				[  --enable-thread-approach enable server thread approach support [[default=no]]])
	if test -z "$enable_thread_approach" ; then
		enable_thread_approach="no"
	fi
	if test "$enable_thread_approach" = "yes"; then
      AC_DEFINE(U_SERVER_THREAD_APPROACH_SUPPORT, 1, [enable server thread approach support])
	fi
	AC_MSG_RESULT([$enable_thread_approach])

	AC_MSG_CHECKING(if you want to enable use of precompiled headers)
	AC_ARG_ENABLE(pch,
				[  --enable-pch            enables precompiled header support (currently only gcc >= 3.4) [[default=no]]])
	if test -z "$enable_pch" ; then
		enable_pch="no"
	fi
	AC_MSG_RESULT([$enable_pch])

	AC_MSG_CHECKING(if you want to enable use of coverage)
	AC_ARG_ENABLE(coverage,
  				[  --enable-coverage       enable coverage [[default=no]]])
	if test -z "$enable_coverage" ; then
		enable_coverage="no"
	elif test "$enable_coverage" = "yes" ; then
      CPPFLAGS="${CPPFLAGS} -fprofile-arcs -ftest-coverage"
	fi
	AC_MSG_RESULT([$enable_coverage])

	dnl Check if compile with GCC optimizations flags enabled

	AC_MSG_CHECKING(for compile with GCC optimizations flags enabled)
	AC_ARG_ENABLE(gcc-optimized,
				[  --enable-gcc-optimized  compile with GCC optimizations flags enabled (-finline,-fstrict-aliasing,...) [[default=yes]]])
	if test -z "$enable_gcc_optimized" ; then
		enable_gcc_optimized="yes"
	fi
	AC_MSG_RESULT([${enable_gcc_optimized}])

	dnl Enable debugging via mudflap.
	dnl All code, as far as possible, is compiled instrumented to catch all the bugs valgrind is able to catch.

	AC_MSG_CHECKING(for build binaries with mudflap instrumentation)
	AC_ARG_ENABLE(mudflap,
				[  --enable-mudflap        build binaries with mudflap instrumentation [[default=no]]])
	if test -z "$enable_mudflap" ; then
		enable_mudflap="no"
	fi
	if test "x$enable_mudflap" = xyes; then
		# Check whether the compiler support -fmudflap.
		use_mudflap=no
		old_CFLAGS="$CFLAGS"
		CFLAGS="$CFLAGS -fmudflap"
		AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[]])],[use_mudflap=yes],[use_mudflap=fail])
		CFLAGS="$old_CFLAGS"
		if test "$use_mudflap" = fail; then
			AC_MSG_FAILURE([--enable-mudflap requires a compiler which understands this option])
		fi
	fi
	if test "x$enable_mudflap" = xyes; then
		enable_gcc_optimized="no"
		LDFLAGS="${LDFLAGS} -lmudflap -rdynamic"
		CPPFLAGS="${CPPFLAGS} -fmudflap -funwind-tables"
	fi
	AC_MSG_RESULT([${enable_mudflap}])

	AC_MSG_CHECKING(if you want to enable mode final for build of ulib library)
	AC_ARG_ENABLE(final,
				[  --enable-final          build size optimized apps (needs more amounts of memory) [[default=yes]]])
	if test -z "$enable_final" ; then
		enable_final="yes"
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

  dnl Check if compiler understands the C99 feature of restricted pointers,
  dnl (TWO UNRELATED TYPES CAN'T POINT TO THE SAME MEMORY, ONLY CHAR* HAS THIS PRIVILEGE)
  dnl specified with the __restrict__, or __restrict  type qualifier

  AC_C_RESTRICT
])
