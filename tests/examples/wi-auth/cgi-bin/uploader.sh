#!/bin/sh

# uploader.sh

. ./.env

# set -x

if [ $# -eq 1 -a \
     "$REQUEST_METHOD" = "POST" ]; then

	# -----------------------------------------
	# $1 -> path file upload
	# -----------------------------------------

	mv $1 $HISTORICAL_LOG_DIR

	OUTPUT="<html><body>OK</body></html>"

fi

write_OUTPUT
