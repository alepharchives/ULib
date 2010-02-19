#!/bin/sh

# uploader.sh

. ./.env

if [ "$REQUEST_METHOD" = "POST" -a $# -eq 1 ]; then

	upload_log_ap "$1"

fi

write_OUTPUT "$OUTPUT"

exit 1
