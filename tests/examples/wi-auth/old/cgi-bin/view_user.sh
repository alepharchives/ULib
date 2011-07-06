#!/bin/bash

# view_user.sh

. ./.env

# set -x

if [ $# -eq 0                  -a \
     "$REQUEST_METHOD" = "GET" -a \
     "$REQUEST_URI"    = "/$BASE_NAME" ]; then

	print_page $BASE_NAME "$BACK_TAG"

elif [ $# -ge 1                   -a \
       "$REQUEST_METHOD" = "POST" -a \
       "$REQUEST_URI"    = "/$BASE_NAME" ]; then

	# $1 -> uid

	BASE_NAME=print_user_data

	ask_to_LDAP ldapsearch "-LLL -b $WIAUTH_USER_BASEDN $LDAP_USER_PARAM" "waUid=$1"

	if [ ! -s $TMPFILE.out ]; then
		message_page "Visualizzazione dati utente: utente non registrato" "Visualizzazione dati utente: $1 non registrato!"
	fi

	WA_ACTIVE=`awk '/^waActive/{print $2}' $TMPFILE.out 2>/dev/null`
	WA_UID=`awk		'/^waUid/{print $2}'		$TMPFILE.out 2>/dev/null`
	WA_CELL=`awk	'/^waCell/{print $2}'	$TMPFILE.out 2>/dev/null`

	ask_to_LDAP ldapsearch "-LLL -b $WIAUTH_CARD_BASEDN $LDAP_CARD_PARAM" "waUsedBy=$1"

	if [ ! -s $TMPFILE.out ]; then
		message_page "Visualizzazione dati utente: utente non attivato" "Visualizzazione dati utente: $1 non attivato!"
	fi

	WA_CID=`awk			'/^waCid/{print $2}'			$TMPFILE.out 2>/dev/null`
	WA_PIN=`awk			'/^waPin/{print $2}'			$TMPFILE.out 2>/dev/null`
	WA_CARDID=`awk		'/^waCardId/{print $2}'		$TMPFILE.out 2>/dev/null`
	WA_REVOKED=`awk	'/^waRevoked/{print $2}'	$TMPFILE.out 2>/dev/null`
	WA_VALIDITY=`awk	'/^waValidity/{print $2}'	$TMPFILE.out 2>/dev/null`
	WA_LOGIN=`awk		'/^waLogin/{print $2}'		$TMPFILE.out 2>/dev/null`
	WA_PASSWORD=`awk	'/^waPassword/{print $2}'	$TMPFILE.out 2>/dev/null`
	WA_USEDBY=`awk		'/^waUsedBy/{print $2}'		$TMPFILE.out 2>/dev/null`
	WA_NOTAFTER=`awk	'/^waNotAfter/{print $2}'	$TMPFILE.out 2>/dev/null`

	YEAR=`echo ${WA_NOTAFTER:0:4}`
	MONTH=`echo ${WA_NOTAFTER:4:2}`
	DAY=`echo ${WA_NOTAFTER:6:2}`
	HOUR=`echo ${WA_NOTAFTER:8:2}`
	MINUTES=`echo ${WA_NOTAFTER:10:2}`

	if [ -s $DIR_CNT/$POLICY/$1.timeout ]; then

		REMAINING_TIME=`cat $DIR_CNT/$POLICY/$1.timeout`

		# expressing the time in minutes
		REMAINING_TIME_MIN=`expr $REMAINING_TIME / 60`
	else
		REMAINING_TIME_MIN="Non disponibile"
	fi

	if [ -s $DIR_CNT/$POLICY/$1.traffic ]; then

		REMAINING_TRAFFIC=`cat $DIR_CNT/$POLICY/$1.traffic`

		# expressing the traffic in MB 1024*1024=1048576
		REMAINING_TRAFFIC_MB=`expr $REMAINING_TRAFFIC / 1048576`
	else
		REMAINING_TRAFFIC_MB="Non disponibile"
	fi

	if [ -z "$WA_NOTAFTER" ]; then
		WA_NOTAFTER="Non disponibile"
	else
		WA_NOTAFTER="$DAY/$MONTH/$YEAR - $HOUR:$MINUTES"
	fi

	REVOKED=NO

	if [ "$WA_REVOKED" = "TRUE" ]; then
		REVOKED=SI
	fi

	print_page $1 "$REMAINING_TIME_MIN" "$REMAINING_TRAFFIC_MB" $WA_PASSWORD "$WA_NOTAFTER" $WA_VALIDITY $REVOKED $WA_CID "$BACK_TAG"

fi

write_OUTPUT
