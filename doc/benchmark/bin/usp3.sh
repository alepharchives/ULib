#!/bin/sh

# usp3.sh

# gcc -DKEEP_ALIVES -DHP_HTTPERF -static ab.c -lpthread -o bench3_keepalive
# gcc					  -DHP_HTTPERF -static ab.c -lpthread -o bench3_NO_keepalive

# const char* IP  = (argv[2]?	    argv[2] :"localhost");
# const char* URL = (argv[3]?	    argv[3] :"/index.html");
# int PORT			= (argv[4]?atoi(argv[4]):80);
# int FROM			= (argv[5]?atoi(argv[5]):0);
# int TO          = (argv[6]?atoi(argv[6]):1000);

ulimit -n 1024

# I had to increase the local port range (because of the TIME_WAIT status of the TCP ports)

# net.ipv4.ip_local_port_range = 1024 65535
/sbin/sysctl -w net/ipv4/ip_local_port_range="1024 65535"

 HOST=$1
#HOST=stefano
#HOST=giallo

mkdir -p		  $HOST && chmod 777	     $HOST
mkdir -p	PERF/$HOST && chmod 777 PERF/$HOST

# httperf --server=$HOST --port=8080 --rate=10 --timeout 5 --hog --uri="/100.html" --num-conns=10    --num-calls 10000 // KEEP-ALIVES
# httperf --server=$HOST --port=8080 --rate=10 --timeout 5 --hog --uri="/100.html" --num-conns=10000 --num-calls 1     // NO Keep_Alives

date > PERF/$1/start.txt

# 100.html
./bench3_keepalive	userver_tcp $HOST "/100.html" 8080 0 1000
mv $1/test.txt			PERF/$1/userver_tcp_100_keepalive.csv
sleep 60
./bench3_NO_keepalive userver_tcp $HOST "/100.html" 8080 0 1000
mv $1/test.txt			 PERF/$1/userver_tcp_100_NO_keepalive.csv

sleep 60

# 1000.html
./bench3_NO_keepalive userver_tcp $HOST "/1000.html" 8080 0 1000
mv $1/test.txt			PERF/$1/userver_tcp_1000_NO_keepalive.csv
sleep 60
./bench3_keepalive	userver_tcp $HOST "/1000.html" 8080 0 1000
mv $1/test.txt			PERF/$1/userver_tcp_1000_keepalive.csv

sleep 60

# servlet/benchmarking?name=stefano
./bench3_NO_keepalive userver_tcp $HOST "/servlet/benchmarking?name=stefano"   8080 0 1000
mv $1/test.txt			PERF/$1/usp_NO_keepalive.csv
sleep 60
./bench3_keepalive   userver_tcp  $HOST "/servlet/benchmarking?name=stefano"   8080 0 1000
mv $1/test.txt			PERF/$1/usp_keepalive.csv

sleep 60

# ws/flash-bridge/WebSocketMain.swf
./bench3_keepalive   userver_tcp  $HOST "/ws/flash-bridge/WebSocketMain.swf"   8080 0 1000
mv $1/test.txt			PERF/$1/userver_tcp_big_keepalive.csv
sleep 60
./bench3_NO_keepalive userver_tcp $HOST "/ws/flash-bridge/WebSocketMain.swf"   8080 0 1000
mv $1/test.txt			 PERF/$1/userver_tcp_big_NO_keepalive.csv

date > PERF/$1/end.txt
