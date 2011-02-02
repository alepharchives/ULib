## Makefile.am for test plugin userver for Fast CGI

MAINTAINERCLEANFILES = Makefile.in

DEFAULT_INCLUDES = -I. -I$(top_srcdir)/include -I..

ulib_la = $(top_builddir)/src/ulib/lib@ULIB@.la

moduledir = $(libexecdir)/ulib

test_fcgi_LDADD   = -lfcgi $(ulib_la)
test_fcgi_SOURCES = test_fcgi.cpp
test_fcgi_LDFLAGS = $(PRG_LDFLAGS)

noinst_PROGRAMS = test_fcgi

clean-local:
	-rm -rf core .libs *.bb* *.da *.gcov *.la *.exe gmon.out
