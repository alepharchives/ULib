#!/bin/sh

. ../.function

start_msg WAGSM_server

rm -f WAGSM_server.log WAGSM/log out/WAGSM_event.sh.out err/WAGSM_event.sh.err \
		out/userver_tcp.out err/userver_tcp.err \
      trace.*userver_tcp*.[0-9]* object.*userver_tcp*.[0-9]*

#UTRACE="0 10M 0"
#UOBJDUMP="0 100k 10"
#USIMERR="error.sim"
 export UTRACE UOBJDUMP USIMERR

DIR_CMD="../../examples/userver"

start_prg_background userver_tcp -c WAGSM_server.cfg
$SLEEP

DIR_CMD="."

mkdir -p /var/spool/sms/outgoing/

start_prg WAGSM_event.sh RECEIVED WAGSM/sms.txt
start_prg WAGSM_event.sh RECEIVED WAGSM/sms_no_card.txt

cat	/var/spool/sms/outgoing/send_* >> out/WAGSM_event.sh.out
rm -f /var/spool/sms/outgoing/send_*

kill_prg userver_tpc TERM

# Test against expected output
test_output_wc c WAGSM_event.sh
