#!/bin/bash

# status_network.sh

. ./.env

# set -x

if [ "$REQUEST_METHOD" = "GET" ]; then

	# --------------------------------------------------------------------------------------------
	# NB: bisogna metterlo in cron (5 minuti)...
	# --------------------------------------------------------------------------------------------
	# get_users_info
	# --------------------------------------------------------------------------------------------

	# stefano 055340773 10.30.1.131:5280 00:e0:4c:d4:63:f5 10.30.1.105

	cat $DIR_CTX/*.ctx 2>/dev/null | sort >/tmp/wi-auth.stat 2>/dev/null

	BODY=`cat $FORM_FILE_DIR/${BASE_NAME}_body.tmpl 2>/dev/null`

	while read AP UUID GATEWAY MAC IP
	do

		RIGA=`printf "$BODY" $UUID $IP $MAC $HTTP_HOST $GATEWAY $AP $AP 2>/dev/null`

		OUTPUT=`echo "$OUTPUT"; echo "$RIGA" 2>/dev/null`

	done < /tmp/wi-auth.stat

	NUM_USERS=`wc -l < /tmp/wi-auth.stat 2>/dev/null`
	NUM_ACCESS_POINT=`wc -l < $ACCESS_POINT_LIST 2>/dev/null`

	TMP1=`cat $FORM_FILE_DIR/${BASE_NAME}_head.tmpl 2>/dev/null`
	TMP2=`date`
	TMP3=`printf "$TMP1" "$TMP2" $NUM_ACCESS_POINT $NUM_USERS 2>/dev/null`

		 write_FILE "$TMP3"																	$DIR_STAT/network.html
	append_to_FILE "$OUTPUT"																$DIR_STAT/network.html
	append_to_FILE "`cat $FORM_FILE_DIR/${BASE_NAME}_end.tmpl 2>/dev/null`" $DIR_STAT/network.html

	echo -e "X-Sendfile: wi-auth/stat/network.html\r\n\r"

	uscita

fi

write_OUTPUT
