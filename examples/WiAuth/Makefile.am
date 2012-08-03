## Makefile.am for examples/IR

DEFAULT_INCLUDES = -I. -I$(top_srcdir)/include

EXTRA_DIST = wi_auth.cpp wi_auth.usp

ulib_la = $(top_builddir)/src/ulib/lib@ULIB@.la

cdbmake_LDADD   = $(top_builddir)/src/ulib/lib@ULIB@.la
cdbmake_SOURCES = cdbmake.cpp
cdbmake_LDFLAGS = $(PRG_LDFLAGS)

bin_PROGRAMS = cdbmake

moduledir = $(libexecdir)/ulib/usp
MODULE_LIBTOOL_OPTIONS = -export-dynamic -avoid-version

if !MINGW
module_LTLIBRARIES = wi_auth.la
wi_auth_la_SOURCES = wi_auth.cpp
wi_auth_la_LDFLAGS = -module $(MODULE_LIBTOOL_OPTIONS)
endif

if !CROSS_COMPILING
SUFFIXES = .cpp .usp .lo .la

wi_auth.cpp: ../../src/ulib/net/server/plugin/usp/usp_translator$(EXEEXT)

.usp.cpp: ../../src/ulib/net/server/plugin/usp/usp_translator$(EXEEXT)
	@$(WINELOADER) ../../src/ulib/net/server/plugin/usp/usp_translator$(EXEEXT) $<

.lo.la:
	$(CXXLINK) -rpath $(moduledir) -module $(MODULE_LIBTOOL_OPTIONS) $< $(ulib_la) $(LIBS)

FOR_ALL = .libs/wi_auth.so

all: $(FOR_ALL)
endif
## ----------------------------------------------------------------------------------------------------

clean-local:
	-rm -rf core .libs *.bb* *.da *.gcov *.la *.exe gmon.out
