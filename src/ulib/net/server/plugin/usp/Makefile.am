## Makefile.am for plugin dynamic page userver

MAINTAINERCLEANFILES = Makefile.in

EXTRA_DIST = *.usp usp2so.sh

DEFAULT_INCLUDES = -I. -I$(top_srcdir)/include

ulib_la = $(top_builddir)/src/ulib/lib@ULIB@.la

usp_translator_LDADD   = $(ulib_la)
usp_translator_SOURCES = usp_translator.cpp
usp_translator_LDFLAGS = $(PRG_LDFLAGS)

noinst_PROGRAMS = usp_translator

clean-local:
	-rm -rf core .libs *.bb* *.da *.gcov *.la *.exe gmon.out

## usp section
## ----------------------------------------------------------------------------------------------------
moduledir = $(libexecdir)/ulib/usp
MODULE_LIBTOOL_OPTIONS = -export-dynamic -avoid-version

SUFFIXES = .cpp .usp .lo .la

upload_progress.cpp: ./usp_translator$(EXEEXT)

.usp.cpp: ./usp_translator$(EXEEXT)
	@$(WINELOADER) ./usp_translator$(EXEEXT) $<

.lo.la:
	$(CXXLINK) -rpath $(moduledir) -module $(MODULE_LIBTOOL_OPTIONS) -no-undefined $< $(ulib_la) $(LIBS)

module_LTLIBRARIES 			= upload_progress.la
upload_progress_la_SOURCES = upload_progress.cpp
upload_progress_la_LIBADD  = $(ulib_la)
upload_progress_la_LDFLAGS = -module $(MODULE_LIBTOOL_OPTIONS)

FOR_ALL = .libs/upload_progress.so

all: $(FOR_ALL)
## ----------------------------------------------------------------------------------------------------
