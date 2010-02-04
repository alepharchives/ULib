#!/bin/bash

# login.sh

. ./.env

set_ENV $0

if [ "$REQUEST_METHOD" = "GET" ]; then

	if [ "$REQUEST_URI" = "/logout" ]; then

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

	if [ $# -ge 4 ]; then

		login_request "$@"

	fi

fi

write_OUTPUT "$OUTPUT"

exit 1
