#!/bin/sh

. ../.function

rm -f hello_world.log* err/hello_world.err \
      out/userver_tcp.out err/userver_tcp.err \
      out/userver_ssl.out err/userver_ssl.err \
		trace.*userver_tcp*.[0-9]* object.*userver_tcp*.[0-9]* \
		trace.*userver_ssl*.[0-9]* object.*userver_ssl*.[0-9]*

#UTRACE="0 30M 0"
#UOBJDUMP="0 100k 10"
#USIMERR="error.sim"
 export UTRACE UOBJDUMP USIMERR

DIR_CMD="../../examples/userver"

CURDIR=`pwd`
cd ../../src/ulib/net/server/plugin/usp/; make hello_world.la || exit 1
cd $CURDIR

mkdir -p usp
cd usp
rm -f *.so
if [ "$TERM" = "msys" ]; then
	cp ../../../src/ulib/.libs/libulib*.dll ..
	cp ../../../src/ulib/net/server/plugin/usp/.libs/hello_world.dll .
else
	ln -sf ../../../src/ulib/net/server/plugin/usp/.libs/hello_world.so
fi
cd $CURDIR

ulimit -n 100000

#STRACE=$TRUSS
 start_prg_background userver_tcp -c benchmarking.cfg
#start_prg_background userver_ssl -c benchmarking_ssl.cfg

# run command on another computer
# ab -f SSL3 -n 100000 -c10 -t 1 https://stefano/usp/hello_world.usp

#$SLEEP
#killall userver_tcp userver_ssl

 mv err/userver_tcp.err err/hello_world.err
#mv err/userver_ssl.err err/hello_world.err
