#!/bin/bash

# login.sh

. ./.env

set_ENV $0

if [ "$REQUEST_METHOD" = "GET" ]; then

	if [ "$REQUEST_URI" = "/$BASE_NAME" ]; then

		if [ $# -eq 0 ]; then

			login_request_from_PIAZZE

		elif [ $# -eq 7 ]; then

			login_request_from_nocat "$@"

		fi

	elif [ "$REQUEST_URI" = "/logout" ]; then

		if [ $# -le 1 ]; then

			request_to_logout_from_user

		elif [ $# -ge 8 ]; then

			TMPFILE=/tmp/info_notified_from_nodog.$$

			echo "$@" > $TMPFILE

			info_notified_from_nodog "$@"

			rm -f $TMPFILE

		fi

	fi

elif [ "$REQUEST_METHOD" = "POST" -a "$REQUEST_URI" = "/$BASE_NAME" ]; then

	if [ $# -eq 4 ]; then

		login_request_from_PIAZZE "$@"

	elif [ $# -eq 10 ]; then # login request from nocat

		# $1	-> mac
		# $2  -> ip
		# $3	-> redirect
		# $4	-> gateway
		# $5	-> timeout
		# $6	-> token
		# $7	-> ap
		# $8	-> uid
		# $9	-> pass
		# $10 -> button

		auth_check_card "$1" "$2" "$3" "$4" "$5" "$6" "$7" realm "$8" "$9" "${10}"

	fi

fi

write_OUTPUT "$OUTPUT"

exit 1
