## Makefile.am -- Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = no-dependencies ## dist-shar dist-zip
ACLOCAL_AMFLAGS  = -I m4

EXTRA_DIST = autoconf configure.help TODO LICENSE* README.md README.LZO cdb README.cdb README.configure *.spec* \
				 ULib.m4 rpm.sh rpmpkgreq.lst rpmpkgreq.lst.suse openwrt \
				 doc/html doc/Doxyfile doc/userver_* doc/bench.c doc/?sp_*

MAINTAINERCLEANFILES = configure aclocal.m4 libtool Makefile.in Makefile INSTALL

SUBDIRS = include src examples contrib tests
DIST_SUBDIRS = include src m4 doc contrib examples tests

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
		$(distdir)/tests/examples/CSP/DB_CA/CA* \
		$(distdir)/tests/examples/CSP/DB_CA/log \
		$(distdir)/tests/examples/CSP/DB_CA/.rnd \
		$(distdir)/tests/examples/IR/benchmark/out \
		$(distdir)/tests/examples/IR/benchmark/log \
		$(distdir)/tests/examples/IR/benchmark/err \
		$(distdir)/tests/examples/IR/benchmark/index.cfg \
		$(distdir)/tests/examples/nocat/Pagine/index.html

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
##	@cd $(top_builddir)/src/ulib/plugin; \
##	 if [ ! -f .libs/mod_rpc.so ]; then \
##	   make .libs/mod_rpc.so   >/dev/null 2>/dev/null; \
##	   make .libs/mod_tsa.so   >/dev/null 2>/dev/null; \
##	   make .libs/mod_http.so  >/dev/null 2>/dev/null; \
##	   make .libs/mod_soap.so  >/dev/null 2>/dev/null; \
##	   make .libs/mod_proxy.so >/dev/null 2>/dev/null; \
##	 fi
##endif
