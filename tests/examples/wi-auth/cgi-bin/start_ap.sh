#!/bin/bash

# start_ap.sh

. ./.env

if [ "$REQUEST_METHOD" = "GET" -a $# -eq 1 ]; then

	info_start_ap "$1"

fi

write_OUTPUT "$OUTPUT"

exit 1
