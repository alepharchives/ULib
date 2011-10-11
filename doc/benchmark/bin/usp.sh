#!/bin/sh

# usp.sh

# gcc -DKEEP_ALIVES -static bench1.c -o bench_keepalive    -lpthread
# gcc               -static bench1.c -o bench_NO_keepalive -lpthread

# const char* IP  = (argv[1]?	    argv[1] :"localhost");
# const char* URL = (argv[2]?	    argv[2] :"/index.html");
# int PORT			= (argv[3]?atoi(argv[3]):80);
# int FROM			= (argv[4]?atoi(argv[4]):0);
# int TO          = (argv[5]?atoi(argv[5]):1000);

# I had to increase the local port range (because of the TIME_WAIT status of the TCP ports)

# net.ipv4.ip_local_port_range = 1024 65535
/sbin/sysctl -w net/ipv4/ip_local_port_range="1024 65535"

 HOST=$1
#HOST=stefano
#HOST=giallo

mkdir -p $HOST && chmod 777 $HOST

# ab -n 1000000 -c 10 -S -d -t 1    -H 'Accept-Encoding: gzip,deflate' "http://$HOST:80/servlet/benchmarking?name=stefano" // NO Keep-Alives
# ab -n 1000000 -c 10 -S -d -t 1 -k -H 'Accept-Encoding: gzip,deflate' "http://$HOST:80/servlet/benchmarking?name=stefano" // KEEP-ALIVES

./bench_keepalive    $HOST "/100.html"                            80 0 1000
mv $1/test.txt $1/userver_tcp_100_keepalive.csv

sleep 60

./bench_NO_keepalive $HOST "/100.html"                            80 0 1000
mv $1/test.txt $1/userver_tcp_100_NO_keepalive.csv

sleep 60

./bench_NO_keepalive $HOST "/1000.html"                           80 0 1000
mv $1/test.txt $1/userver_tcp_1000_NO_keepalive.csv

sleep 60

./bench_keepalive    $HOST "/1000.html"                           80 0 1000
mv $1/test.txt $1/userver_tcp_1000_keepalive.csv

sleep 60

./bench_keepalive    $HOST "/ws/flash-bridge/WebSocketMain.swf"   80 0 1000
mv $1/test.txt $1/userver_tcp_big_keepalive.csv

sleep 60

./bench_NO_keepalive $HOST "/ws/flash-bridge/WebSocketMain.swf"   80 0 1000
mv $1/test.txt $1/userver_tcp_big_NO_keepalive.csv

sleep 60

./bench_NO_keepalive $HOST "/servlet/benchmarking?name=stefano"   80 0 1000
mv $1/test.txt $1/usp_NO_keepalive.csv

sleep 60

./bench_keepalive    $HOST "/servlet/benchmarking?name=stefano"   80 0 1000
mv $1/test.txt $1/usp_keepalive.csv
