#!/bin/sh

. ../.function

DOC_ROOT=.

rm -f web_server_tcc*.log \
      out/userver_tcp.out err/userver_tcp.err \
					 trace.*userver_tcp*.[0-9]*			  object.*userver_tcp*.[0-9]*				 stack.*userver_tcp*.[0-9]* \
      $DOC_ROOT/trace.*userver_ssl*.[0-9]* $DOC_ROOT/object.*userver_ssl*.[0-9]* $DOC_ROOT/stack.*userver_ssl*.[0-9]* \
      $DOC_ROOT/trace.*userver_tcp*.[0-9]* $DOC_ROOT/object.*userver_tcp*.[0-9]* $DOC_ROOT/stack.*userver_tcp*.[0-9]*

 UTRACE="0 50M 0"
 UOBJDUMP="0 10M 5000"
#USIMERR="error.sim"
 export UTRACE UOBJDUMP USIMERR

DIR_CMD="../../examples/userver"

if [ "$TERM" != "cygwin" ]; then
   ( cd ../../src/ulib/net/server/plugin/.libs &&
     ln -sf ../mod_tcc/.libs/mod_tcc.so )
fi

#STRACE=$TRUSS
start_prg_background userver_tcp -c 'web_server_tcc.cfg'

#sleep 6
#kill_prg userver_tcp TERM

mv err/userver_tcp.err err/web_server_tcc.err
