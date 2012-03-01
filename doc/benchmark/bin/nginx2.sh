#!/bin/sh

# nginx2.sh

# gcc -DKEEP_ALIVES -DLIGHTY_WEIGHTTP -static ab.c	-lpthread -o bench2_keepalive
# gcc					  -DLIGHTY_WEIGHTTP -static ab.c	-lpthread -o bench2_NO_keepalive

# const char* IP  = (argv[2]?	    argv[2] :"localhost");
# const char* URL = (argv[3]?	    argv[3] :"/index.html");
# int PORT			= (argv[4]?atoi(argv[4]):80);
# int FROM			= (argv[5]?atoi(argv[5]):0);
# int TO          = (argv[6]?atoi(argv[6]):1000);

# I had to increase the local port range (because of the TIME_WAIT status of the TCP ports)

# net.ipv4.ip_local_port_range = 1024 65535
/sbin/sysctl -w net/ipv4/ip_local_port_range="1024 65535"

 HOST=$1
#HOST=stefano
#HOST=giallo

mkdir -p		      $HOST && chmod 777	       $HOST
mkdir -p	WEIGHTTP/$HOST && chmod 777 WEIGHTTP/$HOST

# weighttp rejects concurrency inferior to thread count
# ------------------------------------------------------------------------------------------------------------------------------------------
# weighttp -n 10000 -c 10 -t 1    -H 'Accept-Encoding: gzip,deflate' "http://$HOST:8080/servlet/benchmarking?name=stefano" // NO Keep-Alives
# weighttp -n 10000 -c 10 -t 1 -k -H 'Accept-Encoding: gzip,deflate' "http://$HOST:8080/servlet/benchmarking?name=stefano" // KEEP-ALIVES
# ------------------------------------------------------------------------------------------------------------------------------------------

# 100.html
./bench2_keepalive	nginx $HOST "/100.html" 8080 0 1000
mv $1/test.txt			WEIGHTTP/$1/nginx_100_keepalive.csv
sleep 60
./bench2_NO_keepalive nginx $HOST "/100.html" 8080 0 1000
mv $1/test.txt			WEIGHTTP/$1/nginx_100_NO_keepalive.csv

sleep 60

# 1000.html
./bench2_NO_keepalive nginx $HOST "/1000.html" 8080 0 1000
mv $1/test.txt			WEIGHTTP/$1/nginx_1000_NO_keepalive.csv
sleep 60
./bench2_keepalive	nginx $HOST "/1000.html" 8080 0 1000
mv $1/test.txt			WEIGHTTP/$1/nginx_1000_keepalive.csv

sleep 60

# servlet/benchmarking?name=stefano
#./bench2_NO_keepalive nginx $HOST "/servlet/benchmarking?name=stefano"   8080 0 1000
#mv $1/test.txt			WEIGHTTP/$1/nginx_NO_keepalive.csv
#sleep 60
#./bench2_keepalive   nginx  $HOST "/servlet/benchmarking?name=stefano"   8080 0 1000
#mv $1/test.txt			WEIGHTTP/$1/nginx_keepalive.csv

#sleep 60

# ws/flash-bridge/WebSocketMain.swf
./bench2_keepalive   nginx  $HOST "/ws/flash-bridge/WebSocketMain.swf"   8080 0 1000
mv $1/test.txt			WEIGHTTP/$1/nginx_big_keepalive.csv
sleep 60
./bench2_NO_keepalive nginx $HOST "/ws/flash-bridge/WebSocketMain.swf"   8080 0 1000
mv $1/test.txt			WEIGHTTP/$1/nginx_big_NO_keepalive.csv
