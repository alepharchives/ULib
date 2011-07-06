#!/bin/bash

# stato_utente.sh

. ./.env

# set -x

if [ "$REQUEST_METHOD" = "GET" ]; then

	get_user_context_connection

	if [ -z "$GATEWAY" ]; then
		message_page "Utente non connesso" "Utente non connesso"
	else
		get_user_nome_cognome

		ask_nodog_status_user

		printf "`cat $FORM_FILE_DIR/${BASE_NAME}.tmpl 2>/dev/null`" "`date`" $AP "$USER" "$OUTPUT" "$BACK_TAG" > $DIR_STAT/$UUID.html 2>/dev/null

		echo -e "X-Sendfile: wi-auth/stat/$UUID.html\r\n\r"
	fi

	uscita

fi

write_OUTPUT
