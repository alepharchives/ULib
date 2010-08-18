## Makefile.am for plugin userver

MAINTAINERCLEANFILES = Makefile.in

DEFAULT_INCLUDES = -I. -I$(top_srcdir)/include

ulib_la = $(top_builddir)/src/ulib/lib@ULIB@.la

moduledir = $(libexecdir)/ulib

module_LTLIBRARIES = mod_rpc.la mod_http.la mod_tsa.la mod_nocat.la mod_echo.la mod_fcgi.la mod_stream.la mod_socket.la

mod_rpc_la_SOURCES   = mod_rpc.cpp
mod_rpc_la_LIBADD    = $(ulib_la)
mod_rpc_la_LDFLAGS   = -module $(MODULE_LIBTOOL_OPTIONS)

mod_http_la_SOURCES  = mod_http.cpp
mod_http_la_LIBADD   = $(ulib_la)
mod_http_la_LDFLAGS  = -module $(MODULE_LIBTOOL_OPTIONS)

mod_tsa_la_SOURCES   = mod_tsa.cpp
mod_tsa_la_LIBADD    = $(ulib_la)
mod_tsa_la_LDFLAGS   = -module $(MODULE_LIBTOOL_OPTIONS)

mod_nocat_la_SOURCES = mod_nocat.cpp
mod_nocat_la_LIBADD  = $(ulib_la)
mod_nocat_la_LDFLAGS = -module $(MODULE_LIBTOOL_OPTIONS)

mod_echo_la_SOURCES  = mod_echo.cpp
mod_echo_la_LIBADD   = $(ulib_la)
mod_echo_la_LDFLAGS  = -module $(MODULE_LIBTOOL_OPTIONS)

mod_fcgi_la_SOURCES  = mod_fcgi.cpp
mod_fcgi_la_LIBADD   = $(ulib_la)
mod_fcgi_la_LDFLAGS  = -module $(MODULE_LIBTOOL_OPTIONS)

mod_stream_la_SOURCES  = mod_stream.cpp
mod_stream_la_LIBADD   = $(ulib_la)
mod_stream_la_LDFLAGS  = -module $(MODULE_LIBTOOL_OPTIONS)

mod_socket_la_SOURCES  = mod_socket.cpp
mod_socket_la_LIBADD   = $(ulib_la)
mod_socket_la_LDFLAGS  = -module $(MODULE_LIBTOOL_OPTIONS)

if EXPAT
module_LTLIBRARIES  += mod_soap.la
mod_soap_la_SOURCES  = mod_soap.cpp
mod_soap_la_LIBADD   = $(ulib_la)
mod_soap_la_LDFLAGS  = -module $(MODULE_LIBTOOL_OPTIONS)
endif

if PCRE
module_LTLIBRARIES  += mod_proxy.la
mod_proxy_la_SOURCES = mod_proxy.cpp mod_proxy_service.cpp
mod_proxy_la_LIBADD  = $(ulib_la)
mod_proxy_la_LDFLAGS = -module $(MODULE_LIBTOOL_OPTIONS)
endif

clean-local:
	-rm -rf core .libs *.bb* *.da *.gcov *.la *.exe gmon.out

##if MINGW
##CXX_LD  = $(CXX) $(LDFLAGS) -Wl,--export-all-symbols -Wl,--dll -Wl,--enable-auto-import -W,rpath,$(pkglibexecdir)
##ulib_so = $(top_builddir)/src/ulib/.libs/exports.o $(top_builddir)/src/ulib/.libs/lib@ULIB@.so
##FOR_ALL = .libs/mod_rpc.so .libs/mod_http.so .libs/mod_tsa.so
##
##.libs/mod_rpc.so: mod_rpc.o
##	$(DLLTOOL) --export-all-symbols -e .libs/mod_rpc_exports.o -l .libs/mod_rpc.lib $<
##	$(CXX_LD) -shared .libs/mod_rpc_exports.o $< -o $@ $(ulib_so) $(LIBS)
##
##.libs/mod_http.so: mod_http.o
##	$(DLLTOOL) --export-all-symbols -e .libs/mod_http_exports.o -l .libs/mod_http.lib $<
##	$(CXX_LD) -shared .libs/mod_http_exports.o $< -o $@ $(ulib_so) $(LIBS)
##
##.libs/mod_tsa.so: mod_tsa.o
##	$(DLLTOOL) --export-all-symbols -e .libs/mod_tsa_exports.o -l .libs/mod_tsa.lib $<
##	$(CXX_LD) -shared .libs/mod_tsa_exports.o $< -o $@ $(ulib_so) $(LIBS)
##
##if EXPAT
##FOR_ALL += .libs/mod_soap.so
##
##.libs/mod_soap.so: mod_soap.o
##	$(DLLTOOL) --export-all-symbols -e .libs/mod_soap_exports.o -l .libs/mod_soap.lib $<
##	$(CXX_LD) -shared .libs/mod_soap_exports.o $< -o $@ $(ulib_so) $(LIBS)
##endif
##
##if PCRE
##FOR_ALL += .libs/mod_proxy.so
##
##.libs/mod_proxy.so: mod_proxy.o
##	$(DLLTOOL) --export-all-symbols -e .libs/mod_proxy_exports.o -l .libs/mod_proxy.lib $<
##	$(CXX_LD) -shared .libs/mod_proxy_exports.o $< -o $@ $(ulib_so) $(LIBS)
##endif
##
##all: $(FOR_ALL)
##endif
