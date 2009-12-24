## Makefile.am -- Process this file with automake to produce Makefile.in
## Copyright (C) 2001 Unirel s.r.l.

MAINTAINERCLEANFILES	= Makefile.in

DEFAULT_INCLUDES = -I. -I$(top_builddir)/include

## DEFS = -DDEBUG_DEBUG @DEFS@

SRC = debug_common.cpp error.cpp error_memory.cpp error_simulation.cpp objectDB.cpp objectIO.cpp trace.cpp

LT_VERSION=@LT_CCXX_VERSION@
RELEASE = $(LT_VERSION) -release $(LT_RELEASE)

## for convenience library
## noinst_LTLIBRARIES = libulibdbg.la

lib_LTLIBRARIES = libulibdbg.la

libulibdbg_la_SOURCES = $(SRC)
libulibdbg_la_LDFLAGS = $(RELEASE) $(LIBRARY)

clean-local:
	-rm -rf core .libs *.bb* *.da *.gcov *.la gmon.out
