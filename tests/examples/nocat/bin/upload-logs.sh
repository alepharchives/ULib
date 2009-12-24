#!/bin/sh

LOG_DIR=/tmp

AP_NAME=`uname -n`

upload_to_authserver() {
    eval authserver=`awk '
       /^[   ]*AUTH_SERVICE_URL[  ]/ { print $2 }' /etc/nodog.conf`

   /usr/sbin/uclient -c /etc/uclient.conf -u $1 "${authserver}wi-auth/cgi-bin/uploader.sh"
}

for file in `ls $LOG_DIR/*.gz`
do
	LOG_RENAMED=$LOG_DIR/${AP_NAME}_`basename $file`

	mv $file $LOG_RENAMED

	upload_to_authserver $LOG_RENAMED

	if [ $? -eq 0 ]; then
		rm $LOG_RENAMED
	fi
done
