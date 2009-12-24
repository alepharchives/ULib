## Makefile.am for examples/cgi

MAINTAINERCLEANFILES = Makefile.in

DEFAULT_INCLUDES = -I. -I$(top_srcdir)/include

cgi_dump_LDADD   = $(top_builddir)/src/ulib/lib@ULIB@.la
cgi_dump_SOURCES = main.cpp
cgi_dump_LDFLAGS = $(PRG_LDFLAGS)

noinst_PROGRAMS = cgi_dump

clean-local:
	-rm -rf core .libs *.bb* *.da *.gcov *.la *.exe gmon.out
