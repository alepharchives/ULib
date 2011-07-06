#!/bin/bash

# get_users_info.sh

. ./.env

if [ $# -eq 0						 -a \
	  "$REQUEST_METHOD" = "GET" -a \
	  "$REMOTE_ADDR"	  = "$PORTAL_IP_ADDRESS" ]; then

#	set -x

	# ------------------------------------------------------------------------------------------------------------------------------------------------
	# stefano 10.30.1.131:5280 00:e0:4c:d4:63:f5 10.30.1.105 http://www.google.com 0 10.30.1.105&1257603166&2a2436611f452f8eebddce4992e88f8d 055340773
	# ------------------------------------------------------------------------------------------------------------------------------------------------
	# cat $DIR_REQ/*.req 2>/dev/null | cut -f 1-2 -d' ' | uniq >/tmp/ACCESS_POINT.lst 2>/dev/null
	# ------------------------------------------------------------------------------------------------------------------------------------------------
	# ACCESS_POINT_LIST=$WIFI_PORTAL_HOME/etc/ACCESS_POINT.lst
	# ------------------------------------------------------------------------------------------------------------------------------------------------

	NUM_ACCESS_POINT=`wc -l < $ACCESS_POINT_LIST 2>/dev/null`

	if [ $NUM_ACCESS_POINT -gt 0 ]; then

		while read AP GATEWAY
		do

			ask_nodog_to_check_for_users_info # we request nodog to check for users logout or disconnect...

		done < $ACCESS_POINT_LIST

	fi

	OUTPUT="<html><body>OK</body></html>"

#	set +x

fi

write_OUTPUT
