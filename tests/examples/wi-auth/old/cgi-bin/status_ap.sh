#!/bin/bash

# status_ap.sh

. ./.env

# set -x

if [ $# -eq 2 -a \
     "$REQUEST_METHOD" = "GET" ]; then

	# $1 -> gateway
	# $2 -> ap

	# we request the status of the indicated gateway...
	# -----------------------------------------------------------------------------
	# NB: we need PREFORK_CHILD > 2
	# -----------------------------------------------------------------------------
	GATEWAY=$1

	send_request_to_nodog "http://$GATEWAY/status" $DIR_STAT/$2.html

	echo -e "X-Sendfile: wi-auth/stat/$2.html\r\n\r"

	uscita

fi

write_OUTPUT
