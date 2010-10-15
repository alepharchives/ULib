#!/bin/sh

. ../.function

DOC_ROOT=/mnt/home/stefano/sito/ridwhan

rm -f web_server.log \
      out/userver_tcp.out err/userver_tcp.err \
					 trace.*userver_tcp*.[0-9]*			  object.*userver_tcp*.[0-9]*				 stack.*userver_tcp*.[0-9]* \
      $DOC_ROOT/trace.*userver_ssl*.[0-9]* $DOC_ROOT/object.*userver_ssl*.[0-9]* $DOC_ROOT/stack.*userver_ssl*.[0-9]* \
      $DOC_ROOT/trace.*userver_tcp*.[0-9]* $DOC_ROOT/object.*userver_tcp*.[0-9]* $DOC_ROOT/stack.*userver_tcp*.[0-9]*

 UTRACE="0 50M 0"
 UOBJDUMP="0 100k 10"
#USIMERR="error.sim"
 export UTRACE UOBJDUMP USIMERR

SOCK1=tmp/fcgi.socket-1
SOCK2=tmp/scgi.socket-1

start_test() {

	CMD=test_fcgi

	PIDS=`ps x | grep $CMD | grep -v grep | awk '{ print $1 }'`

	if [ -z "$PIDS" ]; then
#		rm -f										   $SOCK1
		../../src/ulib/plugin/mod_fcgi/$CMD $SOCK1 2>/tmp/$CMD.err &
	fi
}

#start_test

DIR_CMD="../../examples/userver"

( cd ../../src/ulib/plugin/.libs; ln -sf ../mod_geoip/.libs/mod_geoip.so; ln -sf ../mod_shib/.libs/mod_shib.so )

#STRACE=$TRUSS
start_prg_background userver_tcp -c web_server0.cfg
												# RA/RA.cfg
												# web_server.cfg
												# deployment.properties

chmod 777 $SOCK1

#$SLEEP
#kill_prg userver_tcp TERM

mv err/userver_tcp.err err/web_server.err

# $NC -w 2 127.0.0.1 80 <inp/http/get_geoip.req >>out/web_server.out

#openssl s_client -debug -cert ../ulib/CA/username.crt -key ../ulib/CA/username.key -pass pass:caciucco -CApath ../ulib/CA/CApath -verify 0 -connect localhost:80
