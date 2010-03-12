## Makefile.am for plugin userver for shibboleth

MAINTAINERCLEANFILES = Makefile.in

DEFAULT_INCLUDES = -I. -I$(top_srcdir)/include -I..

CXXFLAGS = @CXXFLAGS@ -fexceptions -frtti

ulib_la = $(top_builddir)/src/ulib/lib@ULIB@.la

moduledir = $(libexecdir)/ulib

module_LTLIBRARIES = mod_shib.la

mod_shib_la_SOURCES = mod_shib.cpp ../mod_proxy_service.cpp
mod_shib_la_LDFLAGS = -module $(MODULE_LIBTOOL_OPTIONS)
mod_shib_la_LIBADD  = -llog4cpp -lshib -lshib-target -lsaml -lxml-security-c -lxerces-c $(ulib_la)

clean-local:
	-rm -rf core .libs *.bb* *.da *.gcov *.la *.exe gmon.out
