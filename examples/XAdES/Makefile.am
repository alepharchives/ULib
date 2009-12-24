## Makefile.am for examples/XAdES

DEFAULT_INCLUDES = -I. -I$(top_srcdir)/include

sign_LDADD   = $(top_builddir)/src/ulib/lib@ULIB@.la
sign_SOURCES = sign.cpp
sign_LDFLAGS = $(PRG_LDFLAGS)

verify_LDADD   = $(top_builddir)/src/ulib/lib@ULIB@.la
verify_SOURCES = verify.cpp
verify_LDFLAGS = $(PRG_LDFLAGS)

noinst_PROGRAMS = sign verify

clean-local:
	-rm -rf core .libs *.bb* *.da *.gcov *.la *.exe gmon.out
