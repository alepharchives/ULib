#!/bin/sh

. ../.function

DIR_ROOT=/srv/ticketing
DOC_ROOT=$DIR_ROOT/www

rm -f $DIR_ROOT/var/ticketing.log /tmp/processCGIRequest.err \
		trace.*userver_tcp*.[0-9]* object.*userver_tcp*.[0-9]* stack.*userver_tcp*.[0-9]* \
 		$DOC_ROOT/trace.*userver_tcp*.[0-9]* $DOC_ROOT/object.*userver_tcp*.[0-9]* $DOC_ROOT/stack.*userver_tcp*.[0-9]*

#UTRACE="0 50M 0"
#UOBJDUMP="0 100k 10"
#USIMERR="error.sim"
UTRACE_LEVEL=5
 export UTRACE UOBJDUMP USIMERR UTRACE_LEVEL

DIR_CMD="../../examples/userver"

#STRACE=$TRUSS
 start_prg_background userver_tcp -c $DIR_ROOT/etc/userver.cfg

#$SLEEP
#kill_prg userver_tcp TERM

mv err/userver_tcp.err err/ticketing.err
