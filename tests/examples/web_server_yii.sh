#!/bin/sh

. ../.function

 DOC_ROOT=yii-testdrive
#DOC_ROOT=/mnt/storage/stefano/source/yii-read-only

rm -f web_server_yii*.log \
      out/userver_tcp.out err/userver_tcp.err \
					 trace.*userver_tcp*.[0-9]*			  object.*userver_tcp*.[0-9]*				 stack.*userver_tcp*.[0-9]* \
      $DOC_ROOT/trace.*userver_ssl*.[0-9]* $DOC_ROOT/object.*userver_ssl*.[0-9]* $DOC_ROOT/stack.*userver_ssl*.[0-9]* \
      $DOC_ROOT/trace.*userver_tcp*.[0-9]* $DOC_ROOT/object.*userver_tcp*.[0-9]* $DOC_ROOT/stack.*userver_tcp*.[0-9]*

 UTRACE="0 50M 0"
#UOBJDUMP="0 10M 5000"
#USIMERR="error.sim"
 export UTRACE UOBJDUMP USIMERR

DIR_CMD="../../examples/userver"

SOCK1=tmp/fcgi.socket

spawn_fcgi() {

   PIDS=`ps x | grep php-cgi | grep -v grep | awk '{ print $1 }'`

   if [ -z "$PIDS" ]; then
		rm -f $SOCK1
		/usr/bin/spawn-fcgi -s $SOCK1 -S -f /usr/bin/php-cgi -C 5 -P /var/run/spawn-fcgi.pid
		chmod 777 $SOCK1
   fi
}

spawn_fcgi

#STRACE=$TRUSS
start_prg_background userver_tcp -c web_server_yii.cfg

#sleep 6
#kill_prg userver_tcp TERM

mv err/userver_tcp.err err/web_server_yii.err

#kill_prg userver_tcp TERM
