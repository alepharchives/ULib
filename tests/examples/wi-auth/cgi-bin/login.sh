#!/bin/bash

# login.sh

. ./.env

# set -x

if [ "$REQUEST_METHOD" = "GET" ]; then

	if [ "$REQUEST_URI" = "/logout" ]; then

		if [ $# -le 1 ]; then

			unset BACK_TAG

			# NoDog data saved on file

			REQ_FILE=$DIR_REQ/$SESSION_ID.req

			if [ ! -s $REQ_FILE ]; then
				message_page "ID di sessione mancante" "Utente non connesso (session id: $SESSION_ID)"
			fi

			# ------------------------
			# 1 -> ap
			# 2 -> gateway
			# 3 -> mac
			# 4 -> ip
			# 5 -> redirect
			# 6 -> timeout
			# 7 -> token
			# 8 -> uuid
			# ------------------------
			# stefano 10.30.1.131:5280 00:e0:4c:d4:63:f5 10.30.1.105 http://www.google.com 0 10.30.1.105&1257603166&2a2436611f452f8eebddce4992e88f8d 055340773
			# ------------------------

			read AP GATEWAY MAC IP REDIRECT TIMEOUT TOKEN UUID < $REQ_FILE

			if [ -n "$UUID" -a \
				  -s $DIR_CTX/$UUID.ctx ]; then

				# --------------------------------------------------------------------
				# GET POLICY FROM FILE (UUID_TO_LOG POLICY MAX_TIME MAX_TRAFFIC)
				# --------------------------------------------------------------------
				FILE_UID=$DIR_REQ/$UUID.uid

				read UUID_TO_LOG POLICY MAX_TIME MAX_TRAFFIC < $FILE_UID

				load_policy
				# --------------------------------------------------------------------

				ask_nodog_to_logout_user

				PARAM=""

				BASE_NAME=ringraziamenti

				if [ -s $DIR_CNT/$POLICY/$UUID_TO_LOG.timeout -o \
					  -s $DIR_CNT/$POLICY/$UUID_TO_LOG.traffic ]; then

					PARAM="<table class=\"centered\" border=\"1\">
							<tr>
								<th class=\"header\" colspan=\"2\" align=\"left\">Utente&nbsp;&nbsp;&nbsp;$UUID</th>
							</tr>"

					if [ -s $DIR_CNT/$POLICY/$UUID_TO_LOG.timeout ]; then

						REMAINING_TIME=`cat $DIR_CNT/$POLICY/$UUID_TO_LOG.timeout`

						# expressing the time in minutes
						REMAINING_TIME_MIN=`expr $REMAINING_TIME / 60`

						PARAM="$PARAM
							<tr>
								<td class=\"header\">Tempo residuo (min.)</td>
								<td class=\"data_italic\">$REMAINING_TIME_MIN</td>
							</tr>"
					fi

					if [ -s $DIR_CNT/$POLICY/$UUID_TO_LOG.traffic ]; then

						REMAINING_TRAFFIC=`cat $DIR_CNT/$POLICY/$UUID_TO_LOG.traffic`

						# expressing the traffic in MB 1024*1024=1048576
						REMAINING_TRAFFIC_MB=`expr $REMAINING_TRAFFIC / 1048576`

						PARAM="$PARAM
							<tr>
								<td class=\"header\">Traffico residuo (MB)</td>
								<td class=\"data_italic\">$REMAINING_TRAFFIC_MB</td>
							</tr>"
					fi

					PARAM="$PARAM
							</table>"
				fi

				print_page "$PARAM"

			else
				message_page "Utente non connesso" "Utente non connesso"
			fi

		elif [ $# -ge 8 ]; then

			TMPFILE=/tmp/info_notified_from_nodog.$$

			echo "$@" > $TMPFILE

			info_notified_from_nodog "$@"

			rm -f $TMPFILE

		fi

	fi

elif [ $# -ge 4								  -a \
		 "$REQUEST_METHOD" = "POST"		  -a \
		 "$REQUEST_URI"	 = "/$BASE_NAME" ]; then

		unset BACK_TAG

		# NoDog data saved on file

		REQ_FILE=$DIR_REQ/$SESSION_ID.req

		if [ ! -s $REQ_FILE ]; then
			login_with_problem
		fi

		# ------------------------
		# 1 -> ap
		# 2 -> gateway
		# 3 -> mac
		# 4 -> ip
		# 5 -> redirect
		# 6 -> timeout
		# 7 -> token
		# 8 -> uuid
		# ------------------------
		# stefano 10.30.1.131:5280 00:e0:4c:d4:63:f5 10.30.1.105 http://www.google.com 0 10.30.1.105&1257603166&2a2436611f452f8eebddce4992e88f8d 055340773
		# ------------------------

		read AP GATEWAY MAC IP REDIRECT TIMEOUT TOKEN UUID < $REQ_FILE

		if [ -n "$UUID" ]; then
			FILE_CTX=$DIR_CTX/$UUID.ctx

			if [ ! -s $FILE_CTX ]; then
				anomalia 9
			fi

			message_page "Login" "Sei gia' loggato! (login_request)"
		fi

		UUID_TO_APPEND=1

		# $1 -> realm (10_piazze, paas, ...)
		# $2 -> uid
		# $3 -> password
		# $4 -> bottone

		if [ "$1" = "paas" ]; then

			OP=AUTH_PAAS

			# TODO:
			# ---------------------------------------------------------------------------------
			# ask_to_LDAP ldapsearch ...
			# ---------------------------------------------------------------------------------

			if [ ! -s $TMPFILE.out ]; then
			 	message_page "Errore" "Errore Autorizzazione dominio PASS"
			fi

		else

			OP=PASS_AUTH

			if [ -z "$2" -o -z "$3" ]; then
				message_page "Impostare utente e/o password" "Impostare utente e/o password"
			fi

			# Check 1: Wrong user and/or password

			ask_to_LDAP ldapsearch "-LLL -b $WIAUTH_CARD_BASEDN $LDAP_CARD_PARAM" "(&(waLogin=$2)(waPassword=$3))"

			if [ ! -s $TMPFILE.out ]; then
				message_page "Utente e/o Password errato/i" "Credenziali errate!"
			fi

			# --------------------------------------------------------------------
			# GET USER FOR THIS CARD
			# --------------------------------------------------------------------
			WA_UID=`grep 'waUsedBy: ' $TMPFILE.out | cut -f2 -d' ' 2>/dev/null`

			# Check 2: Activation required

			if [ -z "$WA_UID" ]; then
				message_page "Attivazione non effettuata" "Per utilizzare il servizio e' richiesta l'attivazione"
			fi
			# --------------------------------------------------------------------

			# Check 3: Card revoked

			REVOKED=`grep 'waRevoked: ' $TMPFILE.out | cut -f2 -d' ' 2>/dev/null`

			if [ "$REVOKED" != "FALSE" ]; then
				message_page "Carta revocata" "La tua carta e' revocata!"
			fi

			# --------------------------------------------------------------------
			# GET POLICY FOR THIS CARD
			# --------------------------------------------------------------------
			WA_POLICY=`grep 'waPolicy: ' $TMPFILE.out | cut -f2 -d' ' 2>/dev/null`

			if [ -n "$WA_POLICY" ]; then
				POLICY=$WA_POLICY
			fi

			load_policy

			WA_TIME=`grep 'waTime: '		 $TMPFILE.out | cut -f2 -d' ' 2>/dev/null`
			WA_TRAFFIC=`grep 'waTraffic: ' $TMPFILE.out | cut -f2 -d' ' 2>/dev/null`

			if [ -n "$WA_TIME" ]; then
				MAX_TIME=$WA_TIME
			fi

			if [ -n "$WA_TRAFFIC" ]; then
				MAX_TRAFFIC=$WA_TRAFFIC
			fi
			# --------------------------------------------------------------------

			if [ $NOTAFTER_CHECK -eq 1 ]; then

				GEN_TIME=`date +%Y%m%d%H%M%SZ` # GeneralizedTime YYYYmmddHH[MM[SS]][(./,)d...](Z|(+/-)HH[MM])

				# Check 4: Expired validity

				NOT_AFTER=`grep 'waNotAfter: ' $TMPFILE.out | cut -f2 -d' ' 2>/dev/null`

				if [ -n "$NOT_AFTER" ]; then

					get_timeout_secs "$GEN_TIME" "$NOT_AFTER"

					if [ $TIMEOUT -lt 0 ]; then
						message_page "Validita' scaduta" "La tua validita' e' scaduta!"
					fi

				else

					OP=FIRST_PASS_AUTH

					# Update card with a new generated waNotAfter

					DN=`grep 'dn: '					$TMPFILE.out | cut -f2 -d' ' 2>/dev/null`
					VALIDITY=`grep 'waValidity: ' $TMPFILE.out | cut -f2 -d' ' 2>/dev/null`

					let "TIMEOUT = $VALIDITY * 86400"

					NOT_AFTER=`date --date="+$VALIDITY days" +%Y%m%d%H%M%SZ 2>/dev/null`

					ask_to_LDAP ldapmodify "-c $LDAP_CARD_PARAM" "
dn: $DN
changetype: modify
add: waNotAfter
waNotAfter: $NOT_AFTER
"
				fi
			fi
		fi

		# redirect back to the gateway appending a signed ticket that will signal NoDog to unlock the firewall...

		# $1	-> mac
		# $2  -> ip
		# $3	-> redirect
		# $4	-> gateway
		# $5	-> timeout
		# $6	-> token
		# $7	-> ap
		# $8	-> uid

		send_ticket "$MAC" $IP "$REDIRECT" $GATEWAY $TIMEOUT "$TOKEN" $AP "$2"
fi

write_OUTPUT
