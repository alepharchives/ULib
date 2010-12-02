#!/bin/bash

# statistics.sh

. ./.env

if [ $# -eq 0 -a \
     "$REQUEST_METHOD" = "GET" ]; then

	print_page "$BACK_TAG"

fi

write_OUTPUT
