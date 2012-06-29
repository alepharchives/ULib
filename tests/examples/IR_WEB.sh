#!/bin/sh

. ../.function

start_msg IR_WEB

DOC_ROOT=IR/WEB

rm -f IR_WEB.log* \
      out/userver_tcp.out err/userver_tcp.err \
		/usr/local/libexec/ulib/session.http.jnl \
					 trace.*userver_tcp*.[0-9]*			  object.*userver_tcp*.[0-9]*				 stack.*userver_tcp*.[0-9]* \
      $DOC_ROOT/trace.*userver_ssl*.[0-9]* $DOC_ROOT/object.*userver_ssl*.[0-9]* $DOC_ROOT/stack.*userver_ssl*.[0-9]* \
      $DOC_ROOT/trace.*userver_tcp*.[0-9]* $DOC_ROOT/object.*userver_tcp*.[0-9]* $DOC_ROOT/stack.*userver_tcp*.[0-9]*

 UTRACE="0 50M 0"
#UOBJDUMP="0 100k 10"
#USIMERR="error.sim"
 export UTRACE UOBJDUMP USIMERR

DIR_CMD="../../examples/userver"

if [ "$TERM" != "cygwin" ]; then
	( cd ../../examples/IR; make ir_web.la || exit 1; )
	( cd $DOC_ROOT; ln -sf ../doc; mkdir -p servlet; cd servlet; rm -f *.so; ln -sf ../../../../../examples/IR/.libs/ir_web.so; )
fi

#STRACE=$TRUSS
start_prg_background userver_tcp -c IR_WEB.cfg

#$SLEEP
#kill_prg userver_tcp TERM

mv err/userver_tcp.err err/IR_WEB.err
