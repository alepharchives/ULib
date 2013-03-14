## Makefile.am for plugin dynamic page userver

MAINTAINERCLEANFILES = Makefile.in

EXTRA_DIST = *.usp usp2so.sh *.cpp README.alldemos usp_compile.sh.in

DEFAULT_INCLUDES = -I. -I$(top_srcdir)/include

ulib_la = $(top_builddir)/src/ulib/lib@ULIB@.la

usp_translator_LDADD   = $(ulib_la)
usp_translator_SOURCES = usp_translator.cpp
usp_translator_LDFLAGS = $(PRG_LDFLAGS)

bin_PROGRAMS = usp_translator

clean-local:
	-rm -rf core .libs *.bb* *.da *.gcov *.la *.exe gmon.out usp_compile.sh

## usp section
## ----------------------------------------------------------------------------------------------------
moduledir = $(libexecdir)/ulib/usp
MODULE_LIBTOOL_OPTIONS = -export-dynamic -avoid-version

if HUPS
if !MINGW
module_LTLIBRARIES 			= upload_progress.la
upload_progress_la_SOURCES = upload_progress.cpp
upload_progress_la_LIBADD  = $(ulib_la)
upload_progress_la_LDFLAGS = -module $(MODULE_LIBTOOL_OPTIONS)
endif
endif

if !CROSS_COMPILING
SUFFIXES = .cpp .usp .lo .la

upload_progress.cpp: ./usp_translator$(EXEEXT)

.usp.cpp: ./usp_translator$(EXEEXT)
	@$(WINELOADER) ./usp_translator$(EXEEXT) $<

.lo.la:
	$(CXXLINK) -rpath $(moduledir) -module $(MODULE_LIBTOOL_OPTIONS) -no-undefined $< $(ulib_la) $(LIBS)

if HUPS
FOR_ALL = .libs/upload_progress.so

all: $(FOR_ALL)
endif
endif
## ----------------------------------------------------------------------------------------------------
