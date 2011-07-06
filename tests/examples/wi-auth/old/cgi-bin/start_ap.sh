#!/bin/bash

# start_ap.sh

. ./.env

# set -x

if [ $# -eq 1 -a \
     "$REQUEST_METHOD" = "GET" ]; then

	# $1 -> ap

	# ------------------------------------------------------------------------------------------------------------------------------------------------
	# *.req (AP GATEWAY MAC IP REDIRECT TIMEOUT TOKEN UUID)
	# ------------------------------------------------------------------------------------------------------------------------------------------------
	# stefano 10.30.1.131:5280 00:e0:4c:d4:63:f5 10.30.1.105 http://www.google.com 0 10.30.1.105&1257603166&2a2436611f452f8eebddce4992e88f8d 055340773
	# ------------------------------------------------------------------------------------------------------------------------------------------------
	# *.cxt (AP UUID GATEWAY MAC IP)
	# ------------------------------------------------------------------------------------------------------------------------------------------------
	# stefano 055340773 10.30.1.131:5280 00:e0:4c:d4:63:f5 10.30.1.105
	# ------------------------------------------------------------------------------------------------------------------------------------------------

	LIST=`grep -l "^$1 " $DIR_REQ/*.req $DIR_REQ/*.uid $DIR_CTX/*.ctx 2>/dev/null`

	if [ -n "$LIST" ]; then

		OP=QUIT
		LIST_SAFE=""

		for FILE in $LIST
		do
			unset GATEWAY

			SUFFIX="${FILE##*.}"

			if [ "$SUFFIX" = "req" ]; then

				read AP GATEWAY MAC IP REDIRECT TIMEOUT TOKEN UUID < $FILE

			elif [ "$SUFFIX" = "ctx" ]; then

				read AP UUID GATEWAY MAC IP < $FILE

			fi

			if [ "$AP" = "$1" -a "$GATEWAY" = "${REMOTE_ADDR}:5280" ]; then

				LIST_SAFE="$FILE $LIST_SAFE"

				if [ "$SUFFIX" = "ctx" ]; then

					write_to_LOG "$UUID" "$AP" "$IP" "$MAC" "$TIMEOUT" "$TRAFFIC"

				fi

			fi

		done

		rm -f $LIST_SAFE

	fi

	OUTPUT="<html><body>OK</body></html>"

fi

write_OUTPUT
