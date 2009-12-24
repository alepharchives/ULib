## Makefile.am -- Process this file with automake to produce Makefile.in

EXTRA_DIST = base/* cgi/* container/* db/* debug/* dynamic/* flex/* internal/* ldap/* mime/* net/* pcre/* \
			 	 replace/* ssl/* ssh/* ui/* utility/* xml/expat/* xml/libxml2/* zip/* \
				 plugin/*.cpp plugin/Makefile.* \
				 plugin/mod_shib/*.cpp plugin/mod_shib/Makefile.* \
				 *.cpp all_c.c ULib.pc.in ULib.rc.in

DEFAULT_INCLUDES = -I. -I$(top_builddir)/include

pkgconfigdir 	= $(libdir)/pkgconfig
pkgconfig_DATA = ULib.pc

## DEFS = -DDEBUG_DEBUG @DEFS@
## base/mkqsort.c base/rsort.c

SRC_C = base/base.c base/base_error.c base/hash.c base/utility.c \
		  base/lzo/minilzo.c \
		  base/coder/cbase64.c base/coder/cquoted_printable.c base/coder/curl_coder.c base/coder/cxml_coder.c base/coder/cescape.c

SRC_CPP = internal/common.cpp internal/objectIO.cpp \
			 ui/dialog.cpp db/cdb.cpp db/rdb.cpp cgi/cgi.cpp cgi/cgi_input.cpp \
			 mime/header.cpp mime/entity.cpp mime/multipart.cpp \
			 container/vector.cpp container/hash_map.cpp container/tree.cpp \
			 utility/interrupt.cpp utility/compress.cpp utility/services.cpp utility/semaphore.cpp utility/base64.cpp \
			 utility/lock.cpp utility/string_ext.cpp utility/socket_ext.cpp utility/uhttp.cpp \
			 net/ipaddress.cpp net/socket.cpp net/ping.cpp \
 			 net/rpc/rpc.cpp net/rpc/rpc_envelope.cpp net/rpc/rpc_fault.cpp net/rpc/rpc_method.cpp net/rpc/rpc_encoder.cpp \
 			 net/rpc/rpc_object.cpp net/rpc/rpc_gen_method.cpp net/rpc/rpc_parser.cpp \
			 net/client/smtp.cpp net/client/ftp.cpp net/client/pop3.cpp net/client/imap.cpp \
			 net/client/http.cpp net/client/client.cpp \
			 net/ipt_ACCOUNT.cpp \
			 query/query_parser.cpp  event/event_time.cpp \
			 timeval.cpp timer.cpp notifier.cpp string.cpp file.cpp process.cpp file_config.cpp log.cpp \
			 options.cpp application.cpp cache.cpp date.cpp url.cpp tokenizer.cpp command.cpp

if DEBUG
SRC_C   += base/base_trace.c
SRC_CPP += debug/trace.cpp debug/debug_common.cpp debug/error.cpp debug/error_memory.cpp \
		     debug/error_simulation.cpp debug/objectDB.cpp
endif

if !GETOPT_LONG
SRC_C += replace/getopt_long.c
endif

if !MREMAP
SRC_C += replace/mremap.c
endif

if !NANOSLEEP
SRC_C += replace/nanosleep.c
endif

if !SENDFILE
SRC_C += replace/sendfile.c
endif

if !STRNDUP
SRC_C += replace/strndup.c
endif

if !SEMAPHORE
SRC_C += replace/sem.c
endif

if !STRPTIME
SRC_C += replace/strptime.c
endif

if !MKDTEMP
SRC_C += replace/mkdtemp.c
endif

if !MEMRCHR
SRC_C += replace/memrchr.c
endif

if !GMTIME_R
SRC_C += replace/gmtime.c
endif

if !TIMEGM
SRC_C += replace/timegm.c
endif

if !FNMATCH
SRC_C += replace/fnmatch.c
endif

if !FALLOCATE
SRC_C += replace/fallocate.c
endif

if !FALLOCATE64
SRC_C += replace/fallocate64.c
endif

if !PREAD
SRC_C += replace/pread.c
endif

if MEMORY_POOL
SRC_CPP += internal/memory_pool.cpp
endif

if PLUGIN
SRC_CPP += dynamic/dynamic.cpp dynamic/plugin.cpp \
			  net/server/server.cpp net/server/client_image.cpp \
			  net/client/client_rdb.cpp net/server/client_image_rdb.cpp net/server/server_rdb.cpp
endif

if ZIP
SRC_C   += base/zip/inflate.c base/zip/dostime.c base/zip/pushback.c base/zip/ziptool.c
SRC_CPP += zip/zip.cpp
endif

if LIBZ
SRC_C   += base/coder/cgzio.c
endif

if USE_PARSER
SRC_CPP += flex/flexer.cpp flex/bison.cpp
endif

if PCRE
SRC_CPP += pcre/pcre.cpp
endif

if SSL
SRC_C   += base/ssl/des3.c base/ssl/dgst.c
SRC_CPP += ssl/certificate.cpp ssl/signature.cpp ssl/pkcs7.cpp ssl/crl.cpp ssl/pkcs10.cpp \
		     ssl/mime/mime_pkcs7.cpp ssl/net/sslsocket.cpp ssl/net/client/ssl_ftp.cpp
if SSL_TS
SRC_CPP += ssl/timestamp.cpp
endif
endif

if SSH
SRC_CPP += ssh/net/sshsocket.cpp
endif

if LDAP
SRC_CPP += ldap/ldap.cpp
endif

if CURL
SRC_CPP += curl/curl.cpp
endif

if EXPAT
SRC_CPP += xml/expat/attribute.cpp xml/expat/element.cpp xml/expat/xml_parser.cpp \
		     xml/soap/soap_fault.cpp xml/soap/soap_gen_method.cpp xml/soap/soap_parser.cpp xml/soap/soap_encoder.cpp
endif

if LIBXML2
SRC_CPP += xml/libxml2/node.cpp xml/libxml2/document.cpp xml/libxml2/schema.cpp
endif

if MAGIC
SRC_CPP += magic/magic.cpp
endif

if MYSQL
SRC_CPP += mysql/mysql.cpp
endif

if LIBEVENT
SRC_CPP += libevent/event.cpp
endif

if MINGW
SRC_C   += base/win32/mingw32.c
else
SRC_CPP += net/unixsocket.cpp
endif

if FINAL
lib@ULIB@_la_SOURCES = all_c.c all_cpp.cpp
else
lib@ULIB@_la_SOURCES = $(SRC_C) $(SRC_CPP)
endif
lib@ULIB@_la_LDFLAGS = $(LIBRARY_LIBTOOL_OPTIONS)

## for convenience library
## noinst_LTLIBRARIES = lib@ULIB@.la

lib_LTLIBRARIES = lib@ULIB@.la

dist-hook:
	find $(distdir) -type f -name \*.c   -exec /utility/expand_tab '{}' ';'
	find $(distdir) -type f -name \*.cpp -exec /utility/expand_tab '{}' ';'

clean-local:
	-rm -rf core .libs *.bb* *.da *.gcov *.la gmon.out

# ---------- Win32 stuff ----------

##if MINGW
##CXX_LD = $(CXX) $(LDFLAGS) -Wl,--export-all-symbols -Wl,--dll -Wl,--enable-auto-import
##
##.libs/lib@ULIB@.so: .libs/lib@ULIB@.a
##	$(DLLTOOL) --export-all-symbols -e .libs/exports.o -l .libs/lib@ULIB@.lib -z .libs/lib@ULIB@.def $<
##	$(CXX_LD) -shared .libs/exports.o .libs/all_c.o .libs/all_cpp.o $< -o $@ $(LIBS)
##
##all: .libs/lib@ULIB@.so
##endif

if MINGW
lib@ULIB@_la_LDFLAGS 	 += -export-symbols $(srcdir)/@ULIB@.def -no-undefined -Wl,ULib-win32-res.o
lib@ULIB@_la_DEPENDENCIES = ULib-win32-res.o
endif

ULib-win32-res.o: ULib.rc
	$(WINDRES) $< $@
	$(DLLTOOL) -export-all-symbols -z $(srcdir)/@ULIB@.def $(srcdir)/.libs/all_c.o $(srcdir)/.libs/all_cpp.o

if MINGW
install-def-file:
	$(INSTALL) $(srcdir)/@ULIB@.def $(DESTDIR)$(libdir)
uninstall-def-file:
	-rm $(DESTDIR)$(libdir)/@ULIB@.def
else
install-def-file:
uninstall-def-file:
endif

if MS_LIB_AVAILABLE
noinst_DATA = @ULIB@-$(VERSION).lib

install-ms-lib:
	$(INSTALL) @ULIB@-$(VERSION).lib $(DESTDIR)$(libdir)

uninstall-ms-lib:
	-rm $(DESTDIR)$(libdir)/@ULIB@-$(VERSION).lib
else
install-ms-lib:
uninstall-ms-lib:
endif

ULib-$(VERSION).lib: libulib-$(VERSION).la $(srcdir)/@ULIB@.def
	lib -name:libulib-$(VERSION)-0.dll -def:$(srcdir)/@ULIB@.def -out:$@

install-data-local: install-ms-lib install-def-file

uninstall-local: uninstall-ms-lib uninstall-def-file
