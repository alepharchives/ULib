#!/bin/sh

LOG_DIR=/tmp

AP_NAME=`uname -n`

upload_to_authserver() {
   for url in `grep '^[ \t]*AUTH_SERVICE_URL[ \t]' /etc/nodog.conf | tr -d \"` ; do
      test $url = AUTH_SERVICE_URL && continue
      /usr/sbin/uclient -c /etc/uclient.conf -u $1 "${url}uploader"
   done
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
