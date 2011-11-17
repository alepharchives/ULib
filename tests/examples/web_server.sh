#!/bin/sh

. ../.function

DOC_ROOT=/mnt/home/stefano/sito/ridwhan

rm -f web_server*.log \
      out/userver_tcp.out err/userver_tcp.err \
					 trace.*userver_tcp*.[0-9]*			  object.*userver_tcp*.[0-9]*				 stack.*userver_tcp*.[0-9]* \
      $DOC_ROOT/trace.*userver_ssl*.[0-9]* $DOC_ROOT/object.*userver_ssl*.[0-9]* $DOC_ROOT/stack.*userver_ssl*.[0-9]* \
      $DOC_ROOT/trace.*userver_tcp*.[0-9]* $DOC_ROOT/object.*userver_tcp*.[0-9]* $DOC_ROOT/stack.*userver_tcp*.[0-9]*

 UTRACE="0 50M 0"
#UOBJDUMP="0 10M 5000"
#USIMERR="error.sim"
 export UTRACE UOBJDUMP USIMERR

SOCK1=tmp/fcgi.socket

start_test() {

	CMD=test_fcgi

	PIDS=`ps x | grep $CMD | grep -v grep | awk '{ print $1 }'`

	if [ -z "$PIDS" ]; then
#		rm -f	$SOCK1
		../../src/ulib/net/server/plugin/mod_fcgi/$CMD $SOCK1 2>/tmp/$CMD.err &
		chmod 777 $SOCK1
	fi
}

#start_test

#/usr/bin/spawn-fcgi -p 8080 -f /usr/bin/php-cgi -C 5 -P /var/run/spawn-fcgi.pid

DIR_CMD="../../examples/userver"

if [ "$TERM" != "cygwin" ]; then
	( mkdir -p servlet; cd servlet;
	  ln -sf ../../../src/ulib/net/server/plugin/usp/.libs/alldemos.so;
	  ln -sf ../../../src/ulib/net/server/plugin/usp/.libs/calc.so;
	  ln -sf ../../../src/ulib/net/server/plugin/usp/.libs/docalc.so;
	  ln -sf ../../../src/ulib/net/server/plugin/usp/.libs/calcajax.so;
	  ln -sf ../../../src/ulib/net/server/plugin/usp/.libs/jsonrequest.so;
	  ln -sf ../../../src/ulib/net/server/plugin/usp/.libs/upload_progress.so;
	  cd ../../../src/ulib/net/server/plugin/usp/;
	  make alldemos.la		  >/dev/null 2>&1 || exit 1;
	  make calc.la				  >/dev/null 2>&1 || exit 1;
	  make docalc.la			  >/dev/null 2>&1 || exit 1;
	  make calcajax.la		  >/dev/null 2>&1 || exit 1;
	  make jsonrequest.la	  >/dev/null 2>&1 || exit 1;
	  make benchmarking.la	  >/dev/null 2>&1 || exit 1;
	  make upload_progress.la >/dev/null 2>&1 || exit 1;
	  test -d ../.libs &&
	  ( cd ../.libs;
	    ln -sf ../mod_shib/.libs/mod_shib.so;
	    ln -sf ../mod_geoip/.libs/mod_geoip.so ) )
fi

#STRACE=$TRUSS
start_prg_background userver_tcp -c 'web_server.cfg'
												# RA/RA.cfg
												# deployment.properties

# HTTP pseudo-streaming for FLV video

#curl -I -s -D -			'http://localhost/test.flv'					-o /dev/null
#curl -I -s -D -			'http://localhost/test.flv'					-o /tmp/test.flv
#curl    -s -v -r0-499	'http://localhost/test.flv'					-o /tmp/test.flv
#curl    -s -D				'http://localhost/test.flv?start=669000'	-o /tmp/test.flv

#sleep 6
#kill_prg userver_tcp TERM

mv err/userver_tcp.err err/web_server.err

# $NC -w 2 127.0.0.1 80 <inp/http/get_geoip.req >>out/web_server.out

#openssl s_client -debug -cert ../ulib/CA/username.crt -key ../ulib/CA/username.key -pass pass:caciucco -CApath ../ulib/CA/CApath -verify 0 -connect localhost:80
