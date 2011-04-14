#!/bin/sh

# csp.sh

# const char* IP  = (argv[1]?	    argv[1] :"localhost");
# const char* URL = (argv[2]?	    argv[2] :"/index.html");
# int PORT			= (argv[3]?atoi(argv[3]):80);
# int FROM			= (argv[4]?atoi(argv[4]):0);

# I had to increase the local port range (because of the TIME_WAIT status of the TCP ports)

# net.ipv4.ip_local_port_range = 1024 65535
/sbin/sysctl -w net/ipv4/ip_local_port_range="1024 65535"

 HOST=stefano
#HOST=giallo

./bench_NO_keepalive $HOST "/csp?hellox&name=stefano" 8080 0
mv test.txt csp_NO_keepalive.csv

sleep 60
./bench_keepalive    $HOST "/csp?hellox&name=stefano" 8080 0
mv test.txt csp_keepalive.csv

sleep 60
./bench_NO_keepalive $HOST "/99.html" 8080 0
mv test.txt gwan_99_NO_keepalive.csv

sleep 60
./bench_keepalive    $HOST "/99.html" 8080 0
mv test.txt gwan_99_keepalive.csv

sleep 60
./bench_NO_keepalive $HOST "/1000.html" 8080 0
mv test.txt gwan_1000_NO_keepalive.csv

sleep 60
./bench_keepalive    $HOST "/1000.html" 8080 0
mv test.txt gwan_1000_keepalive.csv

sleep 60
./bench_NO_keepalive $HOST "/ws/flash-bridge/WebSocketMain.swf" 8080 0
mv test.txt gwan_big_NO_keepalive.csv

sleep 60
./bench_keepalive    $HOST "/ws/flash-bridge/WebSocketMain.swf" 8080 0
mv test.txt gwan_big_keepalive.csv
