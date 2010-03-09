#!/bin/bash

# stato_utente.sh

. ./.env

if [ "$REQUEST_METHOD" = "GET" ]; then

	get_stato_utente

fi

write_OUTPUT "$OUTPUT"

exit 1
