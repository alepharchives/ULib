#!/bin/bash

# index.sh

. ./.env

set_ENV $0

if [ "$REQUEST_METHOD" = "GET" ]; then

	user_welcome "$@"

fi

write_OUTPUT "$OUTPUT"

exit 1
