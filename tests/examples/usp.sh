#!/bin/sh

. ../.function

start_msg usp

rm -f usp.log* \
      out/userver_tcp.out err/userver_tcp.err \
		trace.*userver_tcp*.[0-9]* object.*userver_tcp*.[0-9]*

#UTRACE="0 10M 0"
#UOBJDUMP="0 100k 10"
#USIMERR="error.sim"
 export UTRACE UOBJDUMP USIMERR

DIR_CMD="../../examples/userver"

CURDIR=`pwd`
cd ../../src/ulib/plugin/usp/; ./usp2so.sh >/dev/null 2>&1 || exit 1
cd $CURDIR

( mkdir -p usp; cd usp; for i in `ls ../../../src/ulib/plugin/usp/.libs/*.so`; do ln -sf $i; done )

#STRACE=$TRUSS
start_prg_background userver_tcp -c usp.cfg

#$SLEEP
#kill_prg userver_tcp TERM

mv err/userver_tcp.err err/usp.err
