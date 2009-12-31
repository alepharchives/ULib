#!/bin/sh

. ../.function

start_msg nocat

DOC_ROOT=/srv/wifi-portal/www

rm -f nocat.log \
		/tmp/firewall.err \
      out/userver_tcp.out err/userver_tcp.err \
		trace.*userver_tcp*.[0-9]* object.*userver_tcp*.[0-9]* \
#		$DOC_ROOT/trace.*userver_tcp*.[0-9]* $DOC_ROOT/object.*userver_tcp*.[0-9]*

#UTRACE="0 10M 0"
#UOBJDUMP="0 100k 10"
#USIMERR="error.sim"
 export UTRACE UOBJDUMP USIMERR

DIR_CMD="../../examples/userver"

wget -qO /tmp/wget.out "https://10.30.1.131/start_ap?ap=`uname -n`"

#STRACE=$TRUSS
 start_prg_background userver_tcp -c nocat.cfg
#start_prg_background userver_tcp -c nocat/etc/nodog.conf

#$SLEEP
#kill_prg userver_tcp TERM

mv err/userver_tcp.err err/nocat.err
