## Makefile.am for plugin userver for TCC (Tiny C Compiler)

MAINTAINERCLEANFILES = Makefile.in

DEFAULT_INCLUDES = -I. -I$(top_srcdir)/include -I..

ulib_la = $(top_builddir)/src/ulib/lib@ULIB@.la

moduledir = $(libexecdir)/ulib

module_LTLIBRARIES = mod_tcc.la

mod_tcc_la_SOURCES = mod_tcc.cpp
mod_tcc_la_LDFLAGS = -module $(MODULE_LIBTOOL_OPTIONS)

clean-local:
	-rm -rf core .libs *.bb* *.da *.gcov *.la *.exe gmon.out
