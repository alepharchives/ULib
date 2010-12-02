#!/bin/bash

# card_activation.sh

. ./.env

if [ $# -eq 1						 -a \
	  "$REQUEST_METHOD" = "GET" -a \
	  "$REMOTE_ADDR"	  = "$PORTAL_IP_ADDRESS" ]; then

# 	set -x

	# -----------------------------------------
	# $1 -> CALLER_ID
	# -----------------------------------------

	check_phone_number "$1"

	if [ $EXIT_VALUE -ne 0 ]; then

		logger -p $LOCAL_SYSLOG_SELECTOR "$PORTAL_NAME: $BASE_NAME: $CALLER_ID"

		uscita

	fi

	# Search card by pin

	ask_to_LDAP ldapsearch "-LLL -b $WIAUTH_CARD_BASEDN $LDAP_CARD_PARAM" "waPin=$CALLER_ID"

	if [ ! -s $TMPFILE.out ]; then

		MSG="Utente $CALLER_ID non registrato!"

		logger -p $LOCAL_SYSLOG_SELECTOR "$PORTAL_NAME: $BASE_NAME: $MSG"

		uscita

	fi

	# Verify the card is already activated

	WA_USEDBY=`awk '/^waUsedBy/{print $2}' $TMPFILE.out 2>/dev/null`

	if [ -n "$WA_USEDBY" ]; then

		MSG="Utente $CALLER_ID giá attivato!"

		logger -p $LOCAL_SYSLOG_SELECTOR "$PORTAL_NAME: $BASE_NAME: $MSG"

		uscita

	fi

	WA_CID=`awk '/^waCid/{print $2}'	$TMPFILE.out 2>/dev/null`

	PASSWORD=`apg -a 1 -M n -n 1 -m 6 -x 6` # generate PASSWORD

	# Update card with a new LOGIN/PASSWORD

	ask_to_LDAP ldapmodify "-c $LDAP_CARD_PARAM" "
dn: waCid=$WA_CID,ou=cards,o=unwired-portal
changetype: modify
add: waLogin
waLogin: $CALLER_ID
-
add: waPassword
waPassword: $PASSWORD
-
add: waUsedBy
waUsedBy: $CALLER_ID
-
"

	if [ $EXIT_VALUE -ne 0 ]; then

		MSG="Update card failed!"

		logger -p $LOCAL_SYSLOG_SELECTOR "$PORTAL_NAME: $BASE_NAME: $MSG"

		uscita

	fi

	logger -p $LOCAL_SYSLOG_SELECTOR "$PORTAL_NAME: $BASE_NAME: Login <$CALLER_ID>"
	logger -p $LOCAL_SYSLOG_SELECTOR "$PORTAL_NAME: $BASE_NAME: Password <$PASSWORD>"

	OUTPUT="<html><body>OK</body></html>"

# 	set +x

fi

write_OUTPUT
