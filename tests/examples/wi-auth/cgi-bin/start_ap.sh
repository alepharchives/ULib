#!/bin/bash

# start_ap.sh

. ./.env

set_ENV $0

if [ "$REQUEST_METHOD" = "GET" -a $# -eq 1 ]; then

	info_start_ap "$1"

fi

write_OUTPUT "$OUTPUT"

exit 1
