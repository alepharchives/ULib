#!/bin/bash

# registrazione-business.sh

. ./.env

set_ENV $0

if [ "$REQUEST_METHOD" = "GET" ]; then

	if [ $# -eq 0 ]; then

		view_form_input

	elif [ $# -eq 1 ]; then

		view_page_image $1 

	fi

elif [ "$REQUEST_METHOD" = "POST" ]; then

	if [ $# -eq 25 ]; then

		visualizza_contratto "$@"

	elif [ $# -eq 1 ]; then

		if [ "$1" = "Visualizza contratto" ]; then

			visualizza_contratto

		elif [ "$1" = "Visualizza RID" ]; then

			# codice fiscale
			visualizza_rid 7 TRIMESTRALE

		elif [ "$1" = "Attiva scansione" ]; then

			rascan_image 600

		elif [ "$1" = "Visualizza scansione" ]; then

			view_page_image 1

		elif [ "$1" = "Registra contratto" ]; then

			registrazione_contratto

		fi
	fi
fi

write_OUTPUT "$OUTPUT"

exit 1
