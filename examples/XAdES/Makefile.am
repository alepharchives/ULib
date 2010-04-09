## Makefile.am for examples/XAdES

DEFAULT_INCLUDES = -I. -I$(top_srcdir)/include

sign_LDADD      = $(top_builddir)/src/ulib/lib@ULIB@.la
sign_SOURCES    = sign.cpp
sign_LDFLAGS    = $(PRG_LDFLAGS)

archive_LDADD   = $(top_builddir)/src/ulib/lib@ULIB@.la
archive_SOURCES = archive.cpp
archive_LDFLAGS = $(PRG_LDFLAGS)

client_LDADD    = $(top_builddir)/src/ulib/lib@ULIB@.la
client_SOURCES  = client.cpp
client_LDFLAGS  = $(PRG_LDFLAGS)

verify_LDADD    = libverify.la $(top_builddir)/src/ulib/lib@ULIB@.la
verify_SOURCES  = verify.cpp
verify_LDFLAGS  = $(PRG_LDFLAGS)

libverify_la_SOURCES  = context.cpp transform.cpp xpath.cpp

## for convenience library
noinst_LTLIBRARIES = libverify.la

noinst_PROGRAMS = sign archive client verify

clean-local:
	-rm -rf core .libs *.bb* *.da *.gcov *.la *.exe gmon.out
