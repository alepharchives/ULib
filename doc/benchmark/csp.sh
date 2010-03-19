#!/bin/sh

# csp.sh

#set -x

./csp_NO_keepalive giallo 0 5
mv test.txt csp_NO_keepalive.csv

sleep 60
./csp_keepalive giallo 0 5
mv test.txt csp_keepalive.csv

sleep 60
./1000_NO_keepalive giallo 0 5
mv test.txt gwan_1000_NO_keepalive.csv

sleep 60
./1000_keepalive giallo 0 5
mv test.txt gwan_1000_keepalive.csv
