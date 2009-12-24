#!/bin/sh

# postlogin.sh

if [ "$REQUEST_METHOD" = "GET" -a $# -eq 1 ]; then

	# $1 -> uid

	if [ -n "$HTTP_COOKIE" ]; then

		cat <<END
Content-Type: text/plain; charset=iso-8859-1

UID         = $1
HTTP_COOKIE = $HTTP_COOKIE
END

		exit 0

	fi

fi

exit 1
