## Makefile.am -- Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = no-dependencies

EXTRA_DIST = .colors .function reset.color c_rehash nc-static make_check.sh

MAINTAINERCLEANFILES = Makefile.in

SUBDIRS1 = base
if DEBUG
SUBDIRS1 += debug
endif

SUBDIRS      = $(SUBDIRS1) ulib examples contrib
DIST_SUBDIRS = base debug  ulib examples contrib

dist-hook:
	find $(distdir) -type f -name \*.c   -exec /utility/expand_tab '{}' ';'
	find $(distdir) -type f -name \*.cpp -exec /utility/expand_tab '{}' ';'
	find $(distdir) -type f -name \*.cfg -exec /utility/expand_tab '{}' ';'
