#!/bin/sh

. ../.function

start_msg benchmarking

rm -f benchmarking.log* err/benchmarking.err \
      out/userver_tcp.out err/userver_tcp.err \
      out/userver_ssl.out err/userver_ssl.err \
		trace.*userver_tcp*.[0-9]* object.*userver_tcp*.[0-9]* \
		trace.*userver_ssl*.[0-9]* object.*userver_ssl*.[0-9]*

 UTRACE="0 30M 0"
#UOBJDUMP="0 100k 10"
#USIMERR="error.sim"
 export UTRACE UOBJDUMP USIMERR

DIR_CMD="../../examples/userver"

CURDIR=`pwd`
cd ../../src/ulib/plugin/usp/; make benchmarking.la || exit 1
cd $CURDIR

mkdir -p usp
cd usp
rm -f *.so
if [ "$TERM" = "msys" ]; then
	cp ../../../src/ulib/.libs/libulib*.dll ..
	cp ../../../src/ulib/plugin/usp/.libs/benchmarking.dll .
else
	ln -sf ../../../src/ulib/plugin/usp/.libs/benchmarking.so
fi
cd $CURDIR

#ulimit -n 100000
#echo 1024 > /proc/sys/net/core/somaxconn

#STRACE=$TRUSS
 start_prg_background userver_tcp -c benchmarking.cfg
#start_prg_background userver_ssl -c benchmarking_ssl.cfg

# run command on another computer
# ab -f SSL3 -n 100000 -c10 http://stefano/usp/benchmarking.usp?name=stefano

#$SLEEP
#killall userver_tcp userver_ssl

 mv err/userver_tcp.err err/benchmarking.err
#mv err/userver_ssl.err err/benchmarking.err
