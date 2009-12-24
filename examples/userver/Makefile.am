## Makefile.am for userver

EXTRA_DIST = *.spec* *.start

DEFAULT_INCLUDES = -I. -I$(top_srcdir)/include

userver_tcp_LDADD    = $(top_builddir)/src/ulib/lib@ULIB@.la
userver_tcp_SOURCES  = userver.cpp
userver_tcp_LDFLAGS  = $(PRG_LDFLAGS)
userver_tcp_CPPFLAGS = -DU_TCP_SOCKET $(CPPFLAGS)
noinst_PROGRAMS  		= userver_tcp

if SSL
userver_ssl_LDADD    = $(top_builddir)/src/ulib/lib@ULIB@.la
userver_ssl_SOURCES  = userver.cpp
userver_ssl_LDFLAGS  = $(PRG_LDFLAGS)
userver_ssl_CPPFLAGS = -DU_SSL_SOCKET $(CPPFLAGS)
noinst_PROGRAMS  	  += userver_ssl
endif

## MINGW don't have unix socket...
if !MINGW
userver_ipc_LDADD    = $(top_builddir)/src/ulib/lib@ULIB@.la
userver_ipc_SOURCES  = userver.cpp
userver_ipc_LDFLAGS  = $(PRG_LDFLAGS)
userver_ipc_CPPFLAGS = -DU_UNIX_SOCKET $(CPPFLAGS)
noinst_PROGRAMS  	  += userver_ipc
endif

clean-local:
	-rm -rf core .libs *.bb* *.da *.gcov *.la *.exe gmon.out
