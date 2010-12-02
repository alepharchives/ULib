#!/bin/bash

# admin.sh

.  ./.env

if [ $# -eq 0 -a \
     "$REQUEST_METHOD" = "GET" ]; then

	print_page

fi

write_OUTPUT
