#!/bin/bash

# get_users_info.sh

. ./.env

if [ "$REQUEST_METHOD" = "GET" -a $# -eq 0 -a "$REMOTE_ADDR" = "$PORTAL_IP_ADDRESS" ]; then

	get_users_info

fi

write_OUTPUT "$OUTPUT"

exit 1
