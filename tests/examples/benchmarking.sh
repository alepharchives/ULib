#!/bin/sh

. ../.function

rm -f benchmarking.log* err/benchmarking.err \
      out/userver_tcp.out err/userver_tcp.err \
      out/userver_ssl.out err/userver_ssl.err \
		trace.*userver_tcp*.[0-9]* object.*userver_tcp*.[0-9]* stack.*userver_tcp*.[0-9]* \
		trace.*userver_ssl*.[0-9]* object.*userver_ssl*.[0-9]* stack.*userver_ssl*

#UTRACE="0 50M 0"
#UOBJDUMP="0 1M 100"
#USIMERR="error.sim"
#VALGRIND="valgrind -v --trace-children=yes"
 export UTRACE UOBJDUMP USIMERR VALGRIND

DIR_CMD="../../examples/userver"

if [ "$TERM" != "cygwin" ]; then
   ( mkdir -p usp; cd usp;
     rm -f *.so;
     ln -sf ../../../src/ulib/net/server/plugin/usp/.libs/benchmarking.so;
     cd ../../../src/ulib/net/server/plugin/usp/;
     make benchmarking.la >/dev/null 2>&1 || exit 1;
     test -d ../.libs &&
     ( cd ../.libs;
       ln -sf ../mod_shib/.libs/mod_shib.so;
       ln -sf ../page_speed/.libs/mod_pagespeed.so;
       ln -sf ../mod_geoip/.libs/mod_geoip.so ) )
fi

#ulimit -n 100000
#echo 1024 > /proc/sys/net/core/somaxconn

#STRACE=$TRUSS
#VALGRIND="valgrind --tool=exp-dhat"
#MUDFLAP_OPTIONS="-ignore-reads  -backtrace=8"
 start_prg_background userver_tcp -c benchmarking.cfg
#start_prg_background userver_ssl -c benchmarking_ssl.cfg

# run command on another computer
# ab -f SSL3 -n 100000 -c10 http://stefano/usp/benchmarking.usp?name=stefano

#$SLEEP
#killall userver_tcp userver_ssl

 mv err/userver_tcp.err err/benchmarking.err
#mv err/userver_ssl.err err/benchmarking.err
