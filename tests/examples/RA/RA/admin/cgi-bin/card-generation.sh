#!/bin/sh

# card-generation.sh

.  ./.env

if [ "$REQUEST_METHOD" = "GET" ]; then

	print_page

elif [ "$REQUEST_METHOD" = "POST" -a $# -eq 3 ]; then

	send_mail "$1" "$2"

fi

write_OUTPUT "$OUTPUT"

exit 1
