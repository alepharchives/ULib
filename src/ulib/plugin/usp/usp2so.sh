#!/bin/sh

#cat <<END > mk.tmpl
#include Makefile
#
#moduledir = $(libexecdir)/ulib
#MODULE_LIBTOOL_OPTIONS = -export-dynamic -avoid-version
#
#.SUFFIXES:
#.SUFFIXES: .cpp .usp .lo .la .o .obj
#
#usp.cpp:
#	./usp_translator ($<:.usp=)
#	
#.lo.la:
#	$(CXXLINK) -rpath $(moduledir) -module $(MODULE_LIBTOOL_OPTIONS) $< $(ulib_la) $(LIBS)
#
# %s.cpp: %s.usp
# 	./usp_translator $< 
#
# %s.la: %s.lo $(ulib_la)
# 	$(CXXLINK) -rpath $(moduledir) -module $(MODULE_LIBTOOL_OPTIONS) %s.lo $(ulib_la) $(LIBS)
#
# %s$(EXEEXT): %s.$(OBJEXT) $(ulib_la)
#	@rm -f %s$(EXEEXT)
#	$(CXXLINK) %s.$(OBJEXT) $(ulib_la) $(LIBS)
#END
#
#printf "`cat mk.tmpl`" $1 $1 $1  $1 $1 $1 > $1.mk
#
#make -f $1.mk $1.cpp
#
#cat <<END >> $1.cpp
##include <ulib/net/tcpsocket.h>
#
#int U_EXPORT main(int argc, char* argv[])
#{
#	U_ULIB_INIT(argv);
#
#	U_TRACE(5,"main(%d)", argc)
#
#	UTCPSocket s;
#	UClientImage<UTCPSocket> client_image(&s);
#
#	runDynamicPage(&client_image);
#
#	cout << client_image->wbuffer;
#}
#END
#
#make -f $1.mk $1.la
#
#./$1 > $1.html

for i in *.usp
do
	make $(basename $i .usp).la
done
