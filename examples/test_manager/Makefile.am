## Makefile.am for examples/test_manager

DEFAULT_INCLUDES = -I. -I$(top_srcdir)/include

test_manager_SOURCES = test_manager.cpp
test_manager_LDADD   = $(top_builddir)/src/ulib/lib@ULIB@.la
test_manager_LDFLAGS = $(PRG_LDFLAGS)

noinst_PROGRAMS = test_manager

clean-local:
	-rm -rf core .libs *.bb* *.da *.gcov *.la *.exe gmon.out
