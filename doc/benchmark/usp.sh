#!/bin/sh

# usp.sh

#set -x

./usp_NO_keepalive giallo 0 5
mv test.txt usp_NO_keepalive.csv

sleep 60
./usp_keepalive giallo 0 5
mv test.txt usp_keepalive.csv

sleep 60
./1000_NO_keepalive giallo 0 5
mv test.txt userver_tcp_1000_NO_keepalive.csv

sleep 60
./1000_keepalive giallo 0 5
mv test.txt userver_tcp_1000_keepalive.csv
