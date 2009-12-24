## Makefile.am -- Process this file with automake to produce Makefile.in
## Copyright (C) 2005 Unirel s.r.l.

MAINTAINERCLEANFILES	= Makefile.in

DEFAULT_INCLUDES = -I. -I$(top_builddir)/include

EXTRA_DIST = README all.c

SRC_H = compress.h dostime.h pushback.h zipentry.h zipfile.h
SRC_C = dostime.c inflate.c pushback.c ziptool.c

## for convenience library
## noinst_LTLIBRARIES = libzip.la

if FINAL
libzip_la_SOURCES = $(SRC_H) all.c
else
libzip_la_SOURCES = $(SRC_H) $(SRC_C)
endif

lib_LTLIBRARIES = libzip.la
#libzip_la_LDFLAGS = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) -release $(LT_RELEASE)

clean-local:
	-rm -rf core .libs *.bb* *.da *.gcov *.la gmon.out
