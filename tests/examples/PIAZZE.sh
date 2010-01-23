#!/bin/sh

. ../.function

start_msg PIAZZE

DOC_ROOT=/srv/wifi-portal/www

rm -f PIAZZE.log err/PIAZZE.err \
      out/userver_tcp.out err/userver_tcp.err \
      out/userver_ssl.out err/userver_ssl.err \
		trace.*userver_ssl*.[0-9]* object.*userver_ssl*.[0-9]* \
		trace.*userver_tcp*.[0-9]* object.*userver_tcp*.[0-9]* \
		$DOC_ROOT/trace.*userver_ssl*.[0-9]* $DOC_ROOT/object.*userver_ssl*.[0-9]* \
		$DOC_ROOT/trace.*userver_tcp*.[0-9]* $DOC_ROOT/object.*userver_tcp*.[0-9]* \
		/tmp/processCGIRequest.err

#UTRACE="0 10M 0"
#UOBJDUMP="0 100k 10"
#USIMERR="error.accept"
 export UTRACE UOBJDUMP USIMERR

DIR_CMD="../../examples/userver"

#ifconfig eth0:220 192.168.220.102 up					 2>/dev/null
#ip route add 192.168.220.0/24 via 192.168.220.254  2>/dev/null

 STRACE=$TRUSS
#start_prg_background userver_tcp -c PIAZZE.cfg
 start_prg_background userver_ssl -c PIAZZE.cfg

#$SLEEP
#kill_prg userver_tcp TERM

#mv err/userver_tcp.err err/PIAZZE.err
 mv err/userver_ssl.err err/PIAZZE.err

#ip route del 192.168.220.0/24 via 192.168.220.254  2>/dev/null
#ifconfig eth0:220 192.168.220.102 down				 2>/dev/null
