#!/bin/bash

# reset_policy.sh

. ./.env

# set -x

if [ $# -eq 0						 -a \
	  "$REQUEST_METHOD" = "GET" -a \
	  "$REMOTE_ADDR"	  = "$PORTAL_IP_ADDRESS" ]; then

	load_policy

	for FILE in `ls $DIR_CNT/$POLICY/*.timeout 2>/dev/null`
	do
		write_FILE $MAX_TIME $FILE
	done

	for FILE in `ls $DIR_CNT/$POLICY/*.traffic 2>/dev/null`
	do
		write_FILE $MAX_TRAFFIC $FILE
	done

	OUTPUT="<html><body>OK</body></html>"

fi

write_OUTPUT
