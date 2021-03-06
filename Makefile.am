## Makefile.am -- Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = no-dependencies ## dist-shar dist-zip
ACLOCAL_AMFLAGS  = -I m4

EXTRA_DIST = autoconf cdb configure.help TODO LICENSE* README* *.spec* \
				 ULib.m4 rpm.sh rpmpkgreq.lst rpmpkgreq.lst.suse openwrt \
				 doc/Doxyfile doc/benchmark doc/readme.txt shtool *.awk

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
		$(distdir)/tests/ulib/private/* \
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
		$(distdir)/tests/examples/TSA/TSA/.rnd \
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
  		$(distdir)/src/ulib/net/server/plugin/usp/usp_compile.sh \
  		$(distdir)/src/ulib/net/server/plugin/usp/usp_translator \
		$(distdir)/tests/examples/benchmark/docroot/servlet/*.so \
		$(distdir)/tests/examples/benchmark/docroot/servlet/*.usp \
		$(distdir)/tests/examples/benchmark/libraries/libcaptcha.o
		
README.html: README.md
	@Markdown.pl README.md > README.html

install-data-local:
	@${INSTALL} -m 777 $(top_builddir)/libtool ${bindir}/usp_libtool.sh; \
	 ${INSTALL} -m 777 $(top_builddir)/src/ulib/net/server/plugin/usp/usp_compile.sh ${bindir}
