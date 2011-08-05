#!/bin/sh

# usp.sh

# const char* IP  = (argv[1]?	    argv[1] :"localhost");
# const char* URL = (argv[2]?	    argv[2] :"/index.html");
# int PORT			= (argv[3]?atoi(argv[3]):80);
# int FROM			= (argv[4]?atoi(argv[4]):0);

# I had to increase the local port range (because of the TIME_WAIT status of the TCP ports)

# net.ipv4.ip_local_port_range = 1024 65535
/sbin/sysctl -w net/ipv4/ip_local_port_range="1024 65535"

 HOST=stefano
#HOST=giallo

# ab -n 1000000 -c 10 -S -d -t 1    -H 'Accept-Encoding: gzip,deflate' "http://$HOST:80/usp/benchmarking.usp?name=stefano" // NO Keep-Alives
# ab -n 1000000 -c 10 -S -d -t 1 -k -H 'Accept-Encoding: gzip,deflate' "http://$HOST:80/usp/benchmarking.usp?name=stefano" // KEEP-ALIVES

./bench_keepalive    $HOST "/100.html"                          # 8080 0
mv test.txt userver_tcp_100_keepalive.csv

sleep 60

./bench_NO_keepalive $HOST "/100.html"                          # 8080 0
mv test.txt userver_tcp_100_NO_keepalive.csv

sleep 60

./bench_NO_keepalive $HOST "/1000.html"                         # 8080 0
mv test.txt userver_tcp_1000_NO_keepalive.csv

sleep 60

./bench_keepalive    $HOST "/1000.html"                         # 8080 0
mv test.txt userver_tcp_1000_keepalive.csv

sleep 60

./bench_NO_keepalive $HOST "/usp/benchmarking.usp?name=stefano" # 8080 0
mv test.txt usp_NO_keepalive.csv

sleep 60

./bench_keepalive    $HOST "/usp/benchmarking.usp?name=stefano" # 8080 0
mv test.txt usp_keepalive.csv

sleep 60

./bench_keepalive    $HOST "/ws/flash-bridge/WebSocketMain.swf" # 8080 0
mv test.txt userver_tcp_big_keepalive.csv

sleep 60

./bench_NO_keepalive $HOST "/ws/flash-bridge/WebSocketMain.swf" # 8080 0
mv test.txt userver_tcp_big_NO_keepalive.csv
