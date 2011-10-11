#!/bin/sh

. ../.function

rm -f searchengine.log \
      out/userver_tcp.out err/userver_tcp.err \
		trace.*userver_tcp*.[0-9]* object.*userver_tcp*.[0-9]*

 UTRACE="0 20M 0"
 UOBJDUMP="0 100k 10"
#USIMERR="error.sim"
 export UTRACE UOBJDUMP USIMERR

DIR_CMD="../../examples/userver"

CURDIR=`pwd`
cd ../../examples/IR; make web_cgi.la || exit 1
cd $CURDIR

( mkdir -p servlet; cd servlet; ln -sf ../../../examples/IR/.libs/web_cgi.so seek.so )

#STRACE=$TRUSS
start_prg_background userver_tcp -c searchengine.cfg

#$SLEEP
#kill_prg userver_tcp TERM

mv err/userver_tcp.err err/searchengine.err
