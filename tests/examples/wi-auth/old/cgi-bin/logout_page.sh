#!/bin/bash

# logout_page.sh

. ./.env

#	set -x

if [ "$REQUEST_METHOD" = "GET" ]; then

	print_page

fi

write_OUTPUT
