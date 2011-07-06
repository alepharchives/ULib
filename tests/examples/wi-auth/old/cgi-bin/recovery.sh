#!/bin/bash

# recovery.sh

. ./.env

if [ $# -eq 0                  -a \
     "$REQUEST_METHOD" = "GET" -a \
     "$REQUEST_URI"    = "/$BASE_NAME" ]; then

	print_page $BASE_NAME "$BACK_TAG"

elif [ $# -ge 1                   -a \
       "$REQUEST_METHOD" = "POST" -a \
       "$REQUEST_URI"    = "/$BASE_NAME" ]; then

	BASE_NAME=confirm_page

	print_page "Conferma recovery" "Conferma recovery" "$1" "execute_recovery" "$1" "$BACK_TAG"

fi

write_OUTPUT
