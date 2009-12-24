#!/bin/bash

# card-generation.sh

. ./.env

set_ENV $0

if [ "$REQUEST_METHOD" = "GET" ]; then

	view_form_input

elif [ "$REQUEST_METHOD" = "POST" -a $# -eq 3 ]; then

	send_mail "$1" "$2"

fi

write_OUTPUT "$OUTPUT"

exit 1
