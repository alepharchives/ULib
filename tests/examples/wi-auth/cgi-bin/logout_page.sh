#!/bin/bash

# logout_page.sh

. ./.env

if [ "$REQUEST_METHOD" = "GET" ]; then

	logout_page

fi

write_OUTPUT "$OUTPUT"

exit 1
