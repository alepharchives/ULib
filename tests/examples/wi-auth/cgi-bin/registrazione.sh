#!/bin/bash

# registrazione.sh

. ./.env

# set -x

if [ $# -eq 0 -a \
	  "$REQUEST_METHOD" = "GET" ]; then

	# eventuali dati pre form input

	TMP_FORM_FILE=/tmp/$BASE_NAME.prev

	load_value_session

	# --------------------------------------
	# value to skip: (input type select)
	# --------------------------------------
	# "dati_utente_sesso					= $v5"
	# "dati_utente_tipo_di_documento = $v11"
	# "cellulare_prefisso			   = $v15"
	# --------------------------------------

	# nome				 = "$v1"
	# cognome			 = "$v2"
	# nato a				 = "$v3"
	# data di nascita  = "$v4"
	# codice fiscale	 = "$v6"
	# indirizzo			 = "$v7"
	# comune				 = "$v8"
	# provincia			 = "$v9"
	# cap					 = "${v10}"
	# numero documento = "${v12}"
	# rilasciato da	 = "${v13}"
	# rilasciato il	 = "${v14}"
	# cellulare			 = "${v16}"

	CONNECTION_CLOSE=1

	print_page $BASE_NAME \
			"$v1" \
			"$v2" \
			"$v3" \
			"$v4" \
			"$v6" \
			"$v7" \
			"$v8" \
			"$v9" \
			"${v10}" \
			"${v12}" \
			"${v13}" \
			"${v14}" \
			"${v16}" \
			/wi-auth/form/$HTTP_ACCEPT_LANGUAGE

elif [ $# -eq 17 -a \
		 "$REQUEST_METHOD" = "POST" ]; then

	load_policy

	check_phone_number "${15}${16}" # numero cellulare

	if [ $EXIT_VALUE -ne 0 ]; then
		message_page "$CALLER_ID" "$CALLER_ID"
	fi

	TMP_FORM_FILE=$DIR_REG/$CALLER_ID.reg

	save_value_session "$@"

	ask_to_LDAP ldapadd "$LDAP_USER_PARAM" "
dn: waUid=$CALLER_ID,$WIAUTH_USER_BASEDN
objectClass: top
objectClass: waUser
waActive: TRUE
waUid: $CALLER_ID
waCell: $CALLER_ID
"

	if [ $EXIT_VALUE -eq 68 ]; then
		message_page "Utente già registrato" "Utente già registrato"
	fi

	UUID=`uuidgen 2>/dev/null`

	WA_CARDID=${UUID:24:12}

	ask_to_LDAP ldapadd "$LDAP_CARD_PARAM" "
dn: waCid=$UUID,$WIAUTH_CARD_BASEDN
objectClass: top
objectClass: waCard
waCid: $UUID
waPin: $CALLER_ID
waCardId: $WA_CARDID
waRevoked: FALSE
waValidity: 180
waPolicy: $POLICY
waTime: $MAX_TIME
waTraffic: $MAX_TRAFFIC
"

	if [ $EXIT_VALUE -eq 68 ]; then
		message_page "Utente già registrato" "Utente già registrato (ldap branch card)"
	fi

	BASE_NAME=post_registrazione

	print_page $CALLER_ID "polling_attivazione" $CALLER_ID

fi

write_OUTPUT
