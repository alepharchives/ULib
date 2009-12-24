#!/bin/bash

# postlogin.sh

. ./.env

set_ENV $0

if [ "$REQUEST_METHOD" = "GET" ]; then

	if [ $# -eq 8 ]; then

		post_login "$@"

	elif [ $# -eq 2 ]; then

		logout_popup "$@"

	fi

fi

write_OUTPUT "$OUTPUT"

exit 1
