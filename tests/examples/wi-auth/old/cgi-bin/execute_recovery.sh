#!/bin/bash

# execute_recovery.sh

. ./.env

if [ $# -ge 2                   -a \
     "$REQUEST_METHOD" = "POST" -a \
     "$REQUEST_URI"    = "/$BASE_NAME" ]; then

	# $1 -> uid

	ask_to_LDAP ldapsearch "-LLL -b $WIAUTH_USER_BASEDN $LDAP_USER_PARAM" "waUid=$1" dn:

	if [ ! -s $TMPFILE.out ]; then
		message_page "Recovery utente: utente non registrato" "Recovery utente: $1 non registrato!"
	fi

	USER_DN=`cut -f2 -d':' $TMPFILE.out 2>/dev/null`

	ask_to_LDAP ldapsearch "-LLL -b $WIAUTH_CARD_BASEDN $LDAP_CARD_PARAM" "waPin=$1" dn:

	if [ ! -s $TMPFILE.out ]; then
		message_page "Errore recovery utente" "Errore recovery utente: $1 (card)"
	fi

	CARD_DN=`cut -f2 -d':' $TMPFILE.out 2>/dev/null`

	get_user_context_connection $1

	if [ -n "$AP" ]; then
		ask_nodog_to_logout_user
	fi

	ask_to_LDAP ldapdelete "$LDAP_CARD_PARAM" "$CARD_DN"

	if [ $EXIT_VALUE -ne 0 ]; then
		message_page "Errore" "Errore recovery: fallita cancellazione utente $1 (ldap branch card)"
	fi

	ask_to_LDAP ldapdelete "$LDAP_USER_PARAM" "$USER_DN"

	if [ $EXIT_VALUE -ne 0 ]; then
		message_page "Errore" "Errore recovery: fallita cancellazione utente $1 (ldap branch user)"
	fi

	rm -f $DIR_CTX/"$1".ctx $DIR_STAT/"$1".* $DIR_CNT/$POLICY/"$1".* $DIR_REG/"$1".reg

	logger -p $LOCAL_SYSLOG_SELECTOR "$PORTAL_NAME: $BASE_NAME: User <$1> recovered"

	BACK_TAG="<a href=\"/admin\">TORNA AL MENU</a>"

	message_page "Esito recovery" "Recovery completato!"

fi

write_OUTPUT
