## Makefile.am for examples/uclient

DEFAULT_INCLUDES = -I. -I$(top_srcdir)/include

if SSL
uclient_LDADD   = $(top_builddir)/src/ulib/lib@ULIB@.la
uclient_SOURCES = uclient.cpp
uclient_LDFLAGS = $(PRG_LDFLAGS)

bin_PROGRAMS = uclient
endif

clean-local:
	-rm -rf core .libs *.bb* *.da *.gcov *.la *.exe gmon.out
