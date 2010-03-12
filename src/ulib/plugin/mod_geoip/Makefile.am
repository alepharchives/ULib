## Makefile.am for plugin userver for GeoIP

MAINTAINERCLEANFILES = Makefile.in

DEFAULT_INCLUDES = -I. -I$(top_srcdir)/include -I..

ulib_la = $(top_builddir)/src/ulib/lib@ULIB@.la

moduledir = $(libexecdir)/ulib

module_LTLIBRARIES = mod_geoip.la

mod_geoip_la_SOURCES = mod_geoip.cpp
mod_geoip_la_LDFLAGS = -module $(MODULE_LIBTOOL_OPTIONS)
mod_geoip_la_LIBADD  = -lGeoIP $(ulib_la)

clean-local:
	-rm -rf core .libs *.bb* *.da *.gcov *.la *.exe gmon.out
