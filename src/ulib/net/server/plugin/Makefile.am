## Makefile.am for plugin userver

SUBDIRS = usp

if PAGE_SPEED
SUBDIRS += page_speed
endif

if V8
SUBDIRS += v8
endif

if TCC
if !STATIC_HANDLER_TCC
SUBDIRS += mod_tcc
endif
endif

if GEOIP
if !STATIC_HANDLER_GEOIP
SUBDIRS += mod_geoip
endif
endif

if SSL
if PCRE
if MOD_SHIB
if !STATIC_HANDLER_SHIB
SUBDIRS += mod_shib
endif
endif
endif
endif

EXTRA_DIST = *.cpp Makefile.* \
				 mod_tcc/*.cpp mod_tcc/Makefile.* \
				 mod_shib/*.cpp mod_shib/Makefile.* \
				 mod_geoip/*.cpp mod_geoip/Makefile.* \
				 page_speed/*.cpp page_speed/Makefile.* \
				 v8/*.cpp v8/Makefile.*

MAINTAINERCLEANFILES = Makefile.in

DEFAULT_INCLUDES = -I. -I$(top_srcdir)/include

ulib_la = $(top_builddir)/src/ulib/lib@ULIB@.la

moduledir = $(libexecdir)/ulib

module_LTLIBRARIES =

if !STATIC_HANDLER_RPC
module_LTLIBRARIES  += mod_rpc.la
mod_rpc_la_SOURCES   = mod_rpc.cpp
mod_rpc_la_LIBADD    = $(ulib_la)
mod_rpc_la_LDFLAGS   = -module $(MODULE_LIBTOOL_OPTIONS)
endif

if !STATIC_HANDLER_STREAM
module_LTLIBRARIES    += mod_stream.la
mod_stream_la_SOURCES  = mod_stream.cpp
mod_stream_la_LIBADD   = $(ulib_la)
mod_stream_la_LDFLAGS  = -module $(MODULE_LIBTOOL_OPTIONS)
endif

if !STATIC_HANDLER_NOCAT
module_LTLIBRARIES  += mod_nocat.la
mod_nocat_la_SOURCES = mod_nocat.cpp
mod_nocat_la_LIBADD  = $(ulib_la)
mod_nocat_la_LDFLAGS = -module $(MODULE_LIBTOOL_OPTIONS)
endif

if !STATIC_HANDLER_SOCKET
module_LTLIBRARIES    += mod_socket.la
mod_socket_la_SOURCES  = mod_socket.cpp
mod_socket_la_LIBADD   = $(ulib_la)
mod_socket_la_LDFLAGS  = -module $(MODULE_LIBTOOL_OPTIONS)
endif

if !STATIC_HANDLER_SCGI
module_LTLIBRARIES  += mod_scgi.la
mod_scgi_la_SOURCES  = mod_scgi.cpp
mod_scgi_la_LIBADD   = $(ulib_la)
mod_scgi_la_LDFLAGS  = -module $(MODULE_LIBTOOL_OPTIONS)
endif

if !STATIC_HANDLER_FCGI
module_LTLIBRARIES  += mod_fcgi.la
mod_fcgi_la_SOURCES  = mod_fcgi.cpp
mod_fcgi_la_LIBADD   = $(ulib_la)
mod_fcgi_la_LDFLAGS  = -module $(MODULE_LIBTOOL_OPTIONS)
endif

if PCRE
if !STATIC_HANDLER_PROXY
module_LTLIBRARIES  += mod_proxy.la
mod_proxy_la_SOURCES = mod_proxy.cpp mod_proxy_service.cpp
mod_proxy_la_LIBADD  = $(ulib_la)
mod_proxy_la_LDFLAGS = -module $(MODULE_LIBTOOL_OPTIONS)
endif
endif

if EXPAT
if !STATIC_HANDLER_SOAP
module_LTLIBRARIES  += mod_soap.la
mod_soap_la_SOURCES  = mod_soap.cpp
mod_soap_la_LIBADD   = $(ulib_la)
mod_soap_la_LDFLAGS  = -module $(MODULE_LIBTOOL_OPTIONS)
endif
endif

if !STATIC_HANDLER_SSI
module_LTLIBRARIES += mod_ssi.la
mod_ssi_la_SOURCES  = mod_ssi.cpp
mod_ssi_la_LIBADD   = $(ulib_la)
mod_ssi_la_LDFLAGS  = -module $(MODULE_LIBTOOL_OPTIONS)
endif

if !STATIC_HANDLER_TSA
module_LTLIBRARIES  += mod_tsa.la
mod_tsa_la_SOURCES   = mod_tsa.cpp
mod_tsa_la_LIBADD    = $(ulib_la)
mod_tsa_la_LDFLAGS   = -module $(MODULE_LIBTOOL_OPTIONS)
endif

if !STATIC_HANDLER_HTTP
module_LTLIBRARIES  += mod_http.la
mod_http_la_SOURCES  = mod_http.cpp
mod_http_la_LIBADD   = $(ulib_la)
mod_http_la_LDFLAGS  = -module $(MODULE_LIBTOOL_OPTIONS)
endif

if !STATIC_HANDLER_ECHO
module_LTLIBRARIES  += mod_echo.la
mod_echo_la_SOURCES  = mod_echo.cpp
mod_echo_la_LIBADD   = $(ulib_la)
mod_echo_la_LDFLAGS  = -module $(MODULE_LIBTOOL_OPTIONS)
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
