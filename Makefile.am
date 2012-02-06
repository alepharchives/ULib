## Makefile.am -- Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = no-dependencies ## dist-shar dist-zip
ACLOCAL_AMFLAGS  = -I m4

EXTRA_DIST = autoconf cdb configure.help TODO LICENSE* README* *.spec* \
				 ULib.m4 rpm.sh rpmpkgreq.lst rpmpkgreq.lst.suse openwrt \
				 doc/Doxyfile doc/benchmark doc/readme.txt shtool

MAINTAINERCLEANFILES = configure aclocal.m4 libtool Makefile.in Makefile INSTALL

SUBDIRS  = include src/ulib
if PLUGIN
SUBDIRS += src/ulib/net/server/plugin
endif
SUBDIRS += examples contrib tests

DIST_SUBDIRS = include src/ulib src/ulib/net/server/plugin m4 doc contrib examples tests

## bin_SCRIPTS = ulib-config

aclocaldir = $(datadir)/aclocal
aclocal_DATA = ULib.m4

dist-hook:
	@rm -rf \
		$(distdir)/*.spec \
		$(distdir)/cdb/*.o \
		$(distdir)/cdb/*.a \
		$(distdir)/cdb/rts* \
		$(distdir)/cdb/softlimit \
		$(distdir)/examples/*/*.spec \
		$(distdir)/tests/ulib/tmp/* \
		$(distdir)/tests/ulib/CApath/*.*0 \
		$(distdir)/tests/ulib/ok/timer.ok \
		$(distdir)/tests/ulib/ok/interrupt.ok \
		$(distdir)/tests/ulib/ok/memory_pool.ok \
		$(distdir)/tests/examples/csp.log \
		$(distdir)/tests/examples/tsa.log \
		$(distdir)/tests/examples/RSIGN/log \
		$(distdir)/tests/examples/WAGSM/log \
		$(distdir)/tests/examples/CApath/*.*0 \
		$(distdir)/tests/examples/TSA/log \
		$(distdir)/tests/examples/TSA/DB/log \
		$(distdir)/tests/examples/TSA/DB/.rnd \
		$(distdir)/tests/examples/TSA/response/* \
		$(distdir)/tests/examples/www.sito1.com \
		$(distdir)/tests/examples/CSP/DB_CA/CA* \
		$(distdir)/tests/examples/CSP/DB_CA/log \
		$(distdir)/tests/examples/CSP/DB_CA/.rnd \
		$(distdir)/tests/examples/IR/db* \
		$(distdir)/tests/examples/IR/WEB/doc \
		$(distdir)/tests/examples/IR/WEB/servlet/*.so \
		$(distdir)/tests/examples/IR/benchmark/out \
		$(distdir)/tests/examples/IR/benchmark/log \
		$(distdir)/tests/examples/IR/benchmark/err \
		$(distdir)/tests/examples/IR/benchmark/index.cfg \
  		$(distdir)/src/ulib/net/server/plugin/*.o \
  		$(distdir)/src/ulib/net/server/plugin/*.l? \
  		$(distdir)/src/ulib/net/server/plugin/.deps \
  		$(distdir)/src/ulib/net/server/plugin/.libs \
  		$(distdir)/src/ulib/net/server/plugin/Makefile \
  		$(distdir)/src/ulib/net/server/plugin/*/*.o \
  		$(distdir)/src/ulib/net/server/plugin/*/*.l? \
  		$(distdir)/src/ulib/net/server/plugin/*/.deps \
  		$(distdir)/src/ulib/net/server/plugin/*/.libs \
  		$(distdir)/src/ulib/net/server/plugin/*/Makefile \
  		$(distdir)/src/ulib/net/server/plugin/usp/usp_translator \
		$(distdir)/tests/examples/benchmark/docroot/servlet/*.so \
		$(distdir)/tests/examples/benchmark/docroot/servlet/*.usp \
		$(distdir)/tests/examples/benchmark/libraries/libcaptcha.o

README.html: README.md
	@Markdown.pl README.md > README.html

##install-data-local:
##	 @if test -f /etc/ld.so.conf \
##     -a `echo $(libdir) | grep ^/usr`XXX != "XXX" ; then \
##      if ! grep -q "$(libdir)" /etc/ld.so.conf; then \
##         echo "------------------------------------------------------------------------------" ;\
##         echo "Hmm, looks like you forgot to add $(libdir) to /etc/ld.so.conf." ;\
##         echo "I'll try to fix it for you (but pay attention to libtool's output" ;\
##         echo "next time, ok?)" ;\
##         echo "$(libdir)" >> /etc/ld.so.conf && \
##            echo "Ok, all fixed." \
##            || \
##            (echo "" ;\
##            echo "Well, that didn't work.  Remember to add $(libdir) to" ;\
##            echo "/etc/ld.so.conf if you want to use this library.") ;\
##         echo "------------------------------------------------------------------------------" ;\
##         ldconfig ;\
##      fi ;\
##   fi

##if PLUGIN
##install-exec-hook:
##	@cd $(top_builddir)/src/ulib/net/server/plugin; \
##	 if [ ! -f .libs/mod_rpc.so ]; then \
##	   make .libs/mod_rpc.so   >/dev/null 2>/dev/null; \
##	   make .libs/mod_tsa.so   >/dev/null 2>/dev/null; \
##	   make .libs/mod_http.so  >/dev/null 2>/dev/null; \
##	   make .libs/mod_soap.so  >/dev/null 2>/dev/null; \
##	   make .libs/mod_proxy.so >/dev/null 2>/dev/null; \
##	 fi
##endif
