## Makefile.am -- Process this file with automake to produce Makefile.in
## Copyright (C) 2006 Unirel s.r.l.

MAINTAINERCLEANFILES	= Makefile.in

EXTRA_DIST = win32

DEFAULT_INCLUDES = -I. -I$(top_builddir)/include

## DEFS = -DDEBUG_DEBUG @DEFS@

SRC = base.c base_error.c hash.c utility.c \
		lzo/minilzo.c \
		coder/cbase64.c coder/cquoted_printable.c coder/curl_coder.c coder/cxml_coder.c coder/cescape.c coder/chexdump.c

if DEBUG
SRC += base_trace.c
endif

if ZLIB
SRC += coder/cgzio.c
endif

if SSL
SRC += ssl/cdes3.c
endif

if WIN32
SRC += win32/mingw32.c
##SRC += win32/strtoull.c
LIBRARY = -lwinmm -lws2_32
endif

LT_VERSION=@LT_CCXX_VERSION@
RELEASE = $(LT_VERSION) -release $(LT_RELEASE)

## for convenience library
## noinst_LTLIBRARIES = libulibase.la

lib_LTLIBRARIES = libulibase.la

libulibase_la_SOURCES = $(SRC)
libulibase_la_LDFLAGS = $(RELEASE) $(LIBRARY)

clean-local:
	-rm -rf core .libs *.bb* *.da *.gcov *.la gmon.out
