#!/bin/bash

# cpe.sh

. ./.env

if [ $# -eq 7                  -a \
     "$REQUEST_METHOD" = "GET" -a \
     "$REQUEST_URI"    = "/cpe" ]; then

	user_welcome "$@"

fi

write_OUTPUT
