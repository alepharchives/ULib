#!/bin/sh

# usp.sh

#set -x

# const char* IP  = (argv[1]?	    argv[1] :"localhost");
# const char* URL = (argv[2]?	    argv[2] :"/index.html");
# int PORT			= (argv[3]?atoi(argv[3]):80);
# int FROM			= (argv[4]?atoi(argv[4]):0);

./bench_NO_keepalive giallo "/usp/benchmarking.usp?name=stefano" 8080 0
mv test.txt usp_NO_keepalive.csv

sleep 60
./bench_keepalive    giallo "/usp/benchmarking.usp?name=stefano" 8080 0
mv test.txt usp_keepalive.csv

sleep 60
./bench_NO_keepalive giallo "/1000.html"                         8080 0
mv test.txt userver_tcp_1000_NO_keepalive.csv

sleep 60
./bench_keepalive    giallo "/1000.html"                         8080 0
mv test.txt userver_tcp_1000_keepalive.csv
