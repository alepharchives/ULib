#!/bin/sh

#IP_ADDRESS=`grep IP_ADDRESS /etc/nodog.conf | tr -s ' ' | cut -d' ' -f2`
#AUTH_PORTAL=`grep AuthServiceAddr /etc/nodog.conf | tr -d \\\\ | tr -d \\" | cut -d'=' -f2`

for url in `echo $AUTH_PORTAL`; do
   uclient -c /etc/uclient.conf "${url}/get_config?ap=`uname -n`&key=`cat /etc/nodog.cong.key`" >$1

	if [ $? -eq 0 ]; then
		return
	fi

	sleep 1
done
