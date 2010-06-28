#!/bin/sh

. ../.function

start_msg web_server_ssl

rm -f web_server_ssl.log \
      out/userver_ssl.out err/userver_ssl.err \
		trace.*userver_ssl*.[0-9]* object.*userver_ssl*.[0-9]* stack.*userver_ssl*.[0-9]*

 UTRACE="0 10M 0"
 UOBJDUMP="0 100k 10"
#USIMERR="error.sim"
 export UTRACE UOBJDUMP USIMERR

DIR_CMD="../../examples/userver"

#STRACE=$TRUSS
start_prg_background userver_ssl -c web_server_ssl.cfg

#$SLEEP
#kill_prg userver_ssl TERM

mv err/userver_ssl.err err/web_server_ssl.err

#openssl s_client -debug -cert ../ulib/CA/username.crt -key ../ulib/CA/username.key -pass pass:caciucco -CApath ../ulib/CA/CApath -verify 0 -connect localhost:443
