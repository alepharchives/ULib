#!/bin/sh

. ../.function

start_msg usp

DOC_ROOT=benchmark/docroot

rm -f usp.log* \
      out/userver_tcp.out err/userver_tcp.err \
      /usr/local/libexec/ulib/http_session.jnl \
					 trace.*userver_tcp*.[0-9]*			  object.*userver_tcp*.[0-9]*				 stack.*userver_tcp*.[0-9]* \
      $DOC_ROOT/trace.*userver_ssl*.[0-9]* $DOC_ROOT/object.*userver_ssl*.[0-9]* $DOC_ROOT/stack.*userver_ssl*.[0-9]* \
      $DOC_ROOT/trace.*userver_tcp*.[0-9]* $DOC_ROOT/object.*userver_tcp*.[0-9]* $DOC_ROOT/stack.*userver_tcp*.[0-9]*

 UTRACE="0 50M 0"
#UOBJDUMP="0 100k 10"
#USIMERR="error.sim"
 export UTRACE UOBJDUMP USIMERR

DIR_CMD="../../examples/userver"

if [ "$TERM" != "cygwin" ]; then
	( cd ../../src/ulib/net/server/plugin/usp/; ./usp2so.sh >/tmp/usp2so.sh.out 2>&1 || exit 1 )
	( mkdir -p servlet; cd servlet; rm -f *.so *.usp; for i in `ls ../../../src/ulib/net/server/plugin/usp/.libs/*.so`; do ln -sf $i; done; \
																	  for i in `ls ../../../src/ulib/net/server/plugin/usp/*.usp`;		  do ln -sf $i; done )
   ( cd benchmark/docroot; ln -sf ../../servlet )
fi

#STRACE=$TRUSS
start_prg_background userver_tcp -c usp.cfg

#$SLEEP
#kill_prg userver_tcp TERM

mv err/userver_tcp.err err/usp.err
