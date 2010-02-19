#!/bin/bash

# cpe.sh

. ./.env

if [ "$REQUEST_METHOD" = "GET" -a "$REQUEST_URI" = "/cpe" -a $# -eq 7 ]; then

	user_welcome "$@"

fi

write_OUTPUT "$OUTPUT"

exit 1
