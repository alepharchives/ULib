#!/bin/bash

#-----------------------------------
# START STATISTICS FUNCTION
#-----------------------------------
export_statistics_login_as_csv() {

	COMMAND=cat

	if [ -n "$1" ]; then
		FILE_LOG="$HISTORICAL_LOG_DIR/$1"
		COMMAND=$UNCOMPRESS_COMMAND_HISTORICAL_LOGS
	fi

	TMPFILE=/tmp/statistics_login_$$.csv

	$COMMAND $FILE_LOG | \
	awk '
	/LOGIN/ { a=$8; gsub(",","",a) ; login[a $1]+=1 ; if (!date[$1]) date[$1]+=1 ; if (!ap[a]) ap[a]+=1 }

	END {
		n=asorti(date, sorted_date);

		printf "\"\","; 
		
		for (i = 1; i <= n; i++) {
			printf "\"%s\",", sorted_date[i] 
		}; 
		
		printf "\n"
		
		for (j in ap) { 

			printf "\"AP %s\",", j
			
			for (i = 1; i <= n; i++) {
				printf "%.0f,", login[j sorted_date[i]] 
			}

			printf "\n"
		}
	}
	' > $TMPFILE 2>/dev/null

	HTTP_RESPONSE_BODY="<html><body>OK</body></html>"
	HTTP_RESPONSE_HEADER="X-Sendfile: $TMPFILE\r\n"
}

export_statistics_registration_as_csv() {

	TMPFILE=/tmp/statistics_registration_$$.csv

	ask_to_LDAP ldapsearch "-LLL -b $WIAUTH_CARD_BASEDN $LDAP_CARD_PARAM" "waNotAfter=*" waNotAfter

	awk '
	/^waNotAfter/ {
		year=substr(	$2, 0,4); 
		month=substr(	$2, 5,2); 
		day=substr(		$2, 7,2); 
		hour=substr(	$2, 9,2); 
		minutes=substr($2,11,2); 
		seconds=substr($2,13,2); 

		expire_date=year" "month" "day" "hour" "minutes" "seconds;

		# print expire_date;

		expire_date_in_seconds = mktime(expire_date);

		# print expire_date_in_seconds;

		validity=180*60*60*24;

		date=expire_date_in_seconds-validity;

		# print date;

		date_formatted = strftime("%Y/%m/%d", date);

		# print date_formatted;

		registrations[date_formatted]+=1;
		total+=1;
	}

	END {
		n=asorti(registrations, sorted_registrations);

		printf "\"%s\"", "Data";
		printf ",";
		printf "\"%s\"", "Registrazioni";
		printf "\n";

		for (i = 1; i <= n; i++) {

			printf "\"%s\"", sorted_registrations[i];
			printf ",";
			printf "%.0f", registrations[sorted_registrations[i]];
			printf "\n";
		}

		printf "\"%s\"", "Totale";
		printf ",";
		printf "%.0f", total;
		printf "\n";
	}
	' $TMPFILE.out > $TMPFILE 2>/dev/null

	HTTP_RESPONSE_BODY="<html><body>OK</body></html>"
	HTTP_RESPONSE_HEADER="X-Sendfile: $TMPFILE\r\n"
}

historical_statistics_login() {

	TABLE_TAG_START="<table class=\"centered\" border=\"1\">"
	TABLE_TAG_END="</table>"
	TR_TAG_START="<tr>"
	TR_TAG_END="</tr>"
	TD_HEADER_TAG_START="<td class=\"header_smaller\">"
	TD_DATA_TAG_START="<td class=\"data_smaller\" align=\"right\">"
	TD_TAG_END="</td>"
	TH_TAG_START="<th class=\"header_smaller\">"
	TH_TAG_END="</th>"
	URL_TAG_START="<a href=\"admin_view_statistics_login?file=%s\">%s</a>"

	TABLE="$TABLE_TAG_START\n\t$TH_TAG_START\n\t\tARCHIVI\t$TH_TAG_END\n"

	for file in `ls -rt $HISTORICAL_LOG_DIR/$REGEX_HISTORICAL_LOGS`
	do
		filename=`basename $file`
		TAG=`printf "$URL_TAG_START" $filename $filename`

		TABLE="$TABLE\t$TR_TAG_START\n\t\t$TD_DATA_TAG_START\n\t\t\t$TAG\n\t\t$TD_TAG_END\n\t$TR_TAG_END\n"
	done

	TABLE=`echo -e "$TABLE$TABLE_TAG_END"`

	TITLE_TXT="Storico"

	print_page "Storico" "$TABLE" "$BACK_TAG"
}

view_statistics_login() {

	REQUEST_URI=view_statistics
	HREF_TAG="admin_export_statistics_login_as_csv"

	if [ -n "$1" ]; then
		HREF_TAG="$HREF_TAG?file=$1"
		FILE_LOG=$HISTORICAL_LOG_DIR/$1
		COMMAND=$UNCOMPRESS_COMMAND_HISTORICAL_LOGS
		TITLE_TXT="Numero LOGIN per Access Point: file $1"
	else
		COMMAND=cat
		TITLE_TXT="Numero LOGIN per Access Point"
	fi

	export TABLE_TAG_START="<table class=\"centered\" border=\"1\">"
	export TABLE_TAG_END="</table>"
	export TR_TAG_START="<tr>"
	export TR_TAG_END="</tr>"
	export TD_HEADER_TAG_START="<td class=\"header_smaller\">"
	export TD_HEADER_ALIGNED_TAG_START="<td class=\"header_smaller\" align=\"right\">"
	export TD_DATA_TAG_START="<td class=\"data_smaller\" align=\"right\">"
	export TD_TAG_END="</td>"
	export TH_TAG_START="<th class=\"header_smaller\">"
	export TH_TAG_END="</th>"

	TABLE=`$COMMAND $FILE_LOG | awk '
	BEGIN {

		trTagStart=ENVIRON["TR_TAG_START"];
		trTagEnd=ENVIRON["TR_TAG_END"];

		tdTagHeaderStart=ENVIRON["TD_HEADER_TAG_START"];
		tdTagHeaderAlignedStart=ENVIRON["TD_HEADER_ALIGNED_TAG_START"];

		tdTagDataStart=ENVIRON["TD_DATA_TAG_START"];

		tdTagEnd=ENVIRON["TD_TAG_END"];

		thTagStart=ENVIRON["TH_TAG_START"];
		thTagEnd=ENVIRON["TH_TAG_END"];

		printf "%s\n", ENVIRON["TABLE_TAG_START"];
	}

	/LOGIN/ { a=$8; gsub(",","",a) ; login[a $1]+=1 ; login[a]+=1 ; login[$1]+=1 ; if (!date[$1]) date[$1]+=1 ; if (!ap[a]) ap[a]+=1 }

	END {
		n=asorti(date, sorted_date);

		printf "\t%s\n\t\t%s%s%s\n", trTagStart, thTagStart, thTagEnd, "" ; 

		for (i = 1; i <= n; i++) {
			printf "\t\t%s%s%s\n", thTagStart, sorted_date[i], thTagEnd 
		} 

		printf "\t\t%s%s%s\n", thTagStart, "Totale x AP", thTagEnd 
		printf "\t%s\n", trTagEnd

		for (j in ap) { 

			printf "\t%s\n\t\t%sAP %s%s\n", trTagStart, tdTagHeaderStart, j, tdTagEnd

			for (i = 1; i <= n; i++) {
				printf "\t\t%s%.0f%s\n", tdTagDataStart, login[j sorted_date[i]], tdTagEnd
			}

			printf "\t\t%s%.0f%s\n", tdTagHeaderAlignedStart, login[j], tdTagEnd
			printf "\t%s\n", trTagEnd
		}

		printf "\t%s\n\t\t%sTotale x data%s\n", trTagStart, tdTagHeaderStart, tdTagEnd

		for (i = 1; i <= n; i++) {
			printf "\t\t%s%.0f%s\n", tdTagHeaderAlignedStart, login[sorted_date[i]], tdTagEnd
			totale+=login[sorted_date[i]]
		}

		printf "\t\t%s%.0f%s\n", tdTagHeaderAlignedStart, totale, tdTagEnd
		printf "\t%s\n", trTagEnd
		printf "%s\n", ENVIRON["TABLE_TAG_END"];
	}
	'`

	print_page "$TITLE_TXT" "$TABLE" "$BACK_TAG" "<a class=\"back\" href=\"$HREF_TAG\">Esporta in formato CSV</a>"
}

view_statistics_registration() {

	export TABLE_TAG_START="<table class=\"centered\" border=\"1\">"
	export TABLE_TAG_END="</table>"
	export TR_TAG_START="<tr>"
	export TR_TAG_END="</tr>"
	export TD_HEADER_TAG_START="<td class=\"header_smaller\">"
	export TD_DATA_TAG_START="<td class=\"data_smaller\" align=\"right\">"
	export TD_TAG_END="</td>"
	export TH_TAG_START="<th class=\"header_smaller\">"
	export TH_TAG_END="</th>"

	ask_to_LDAP ldapsearch "-LLL -b $WIAUTH_CARD_BASEDN $LDAP_CARD_PARAM" "waNotAfter=*" waNotAfter

	TABLE=`awk '
	BEGIN {

		trTagStart=ENVIRON["TR_TAG_START"];
		trTagEnd=ENVIRON["TR_TAG_END"];

		tdTagHeaderStart=ENVIRON["TD_HEADER_TAG_START"];
		tdTagDataStart=ENVIRON["TD_DATA_TAG_START"];

		tdTagEnd=ENVIRON["TD_TAG_END"];

		thTagStart=ENVIRON["TH_TAG_START"];
		thTagEnd=ENVIRON["TH_TAG_END"];

		printf "%s\n", ENVIRON["TABLE_TAG_START"];
	}

	/^waNotAfter/ { 
		year=substr($2, 0 , 4); 

		month=substr($2, 5 , 2); 

		day=substr($2, 7 , 2); 

		hour=substr($2, 9 , 2); 

		minutes=substr($2, 11 , 2); 

		seconds=substr($2, 13 , 2); 

		expire_date=year" "month" "day" "hour" "minutes" "seconds;
		# print expire_date;

		expire_date_in_seconds = mktime(expire_date);

		# print expire_date_in_seconds;

		validity=180*60*60*24;

		date=expire_date_in_seconds-validity;

		# print date;

		date_formatted = strftime("%Y/%m/%d", date);

		# print date_formatted;

		registrations[date_formatted]+=1;
		total+=1;
	}

	END {
		n=asorti(registrations, sorted_registrations);

		printf "\t%s\n", trTagStart;
		printf "\t\t%s\n", thTagStart;
		printf "\t\t\t%s\n", "Data";
		printf "\t\t%s\n", thTagEnd;
		printf "\t\t%s\n", thTagStart;
		printf "\t\t\t%s\n", "Registrazioni";
		printf "\t\t%s\n", thTagEnd;
		printf "\t%s\n", trTagEnd;

		for (i = 1; i <= n; i++) {
			printf "\t%s\n", trTagStart;

			printf "\t\t%s\n", tdTagDataStart;
			printf "\t\t\t%s\n", sorted_registrations[i];
			printf "\t\t%s\n", tdTagEnd;

			printf "\t\t%s\n", tdTagDataStart;
			printf "\t\t\t%.0f\n", registrations[sorted_registrations[i]];
			printf "\t\t%s\n", tdTagEnd;

			printf "\t%s\n", trTagEnd;
		}

		printf "\t%s\n", trTagStart;

		printf "\t\t%s\n", tdTagDataStart;
		printf "\t\t\t%s\n", "Totale";
		printf "\t\t%s\n", tdTagEnd;

		printf "\t\t%s\n", tdTagDataStart;
		printf "\t\t\t%.0f\n", total;
		printf "\t\t%s\n", tdTagEnd;

		printf "\t%s\n", trTagEnd;

		printf "%s\n", ENVIRON["TABLE_TAG_END"];
	}
	' $TMPFILE.out`

	REQUEST_URI=view_statistics
	TITLE_TXT="Numero Registrazioni per data"
	HREF_TAG="admin_export_statistics_registration_as_csv"

	print_page "$TITLE_TXT" "$TABLE" "$BACK_TAG" "<a class=\"back\" href=\"$HREF_TAG\">Esporta in formato CSV</a>"
}

printlog() {

	TMPFILE=/tmp/printlog_$$

	# 2009/11/07 13:14:35 op: PASS_AUTH, uid: 3397363258, ap: dev, ip: 10.30.1.105, mac: 00:e0:4c:d4:63:f5, timeout: 86400, traffic: 300

	awk '
	{
	for (f = 1 ; f <= NF ; f++)
		{
		row[NR,f] = $f

		l = length($f) ; if (l > max[f]) max[f] = l
		}

	if (NF > maxNF) maxNF = NF
	}

	END {
		for (r = (NR > 200 ? NR : 201) - 200 ; r <= NR ; r++)
			for (f = 1 ; f <= maxNF ; f++)
				printf "%-*s%s",
								 (max[f] > 999 ? 999 : max[f]),
					(length(row[r,f]) > 999 ? substr(row[r,f], 1, 999 -3) "..." : row[r,f]),
					(f < maxNF ? " " : "\n")
	}
	' $FILE_LOG > $TMPFILE 2>/dev/null

	if [ ! -s "$TMPFILE" ]; then
		echo "EMPTY" > $TMPFILE
	fi

	HTTP_RESPONSE_BODY="<html><body>OK</body></html>"
	HTTP_RESPONSE_HEADER="X-Sendfile: $TMPFILE\r\n"
}
#-----------------------------------
# END STATISTICS FUNCTION
#-----------------------------------

#-----------------------------------
# START FUNCTION
#-----------------------------------
write_FILE() {

	# $1 -> data
	# $2 -> filename
	# $3 -> option

	echo $3 "$1" > $2

	if [ $? -ne 0 ]; then
		anomalia 2
	fi
}

append_to_FILE() {

	# $1 -> data
	# $2 -> filename

	echo "$1" >> $2

	if [ $? -ne 0 ]; then
		anomalia 3
	fi
}

print_page() {

	if [ -r $DIR_TEMPLATE/$REQUEST_URI.tmpl ]; then

		BODY_SHTML=`cat $DIR_TEMPLATE/$REQUEST_URI.tmpl 2>/dev/null`

		if [ $# -ne 0 ]; then
			BODY_SHTML=`printf "$BODY_SHTML" "$@" 2>/dev/null`
		fi
	fi
}

message_page() {

	TITLE_TXT="$1"
	REQUEST_URI=message_page

	shift

	print_page "$@" "$BACK_TAG"

	write_SSI
}

anomalia() {

	# -----------------------------------------------
	# $1 -> exit value
	# ------------------------------------------------------------
	#  2 write_FILE
	#  3 append_to_FILE
	#  4 sign_data
	#  5 send_request_to_nodog (curl | uclient)
	#  6 info_notified_from_nodog (missing ctx)
	#  7 info_notified_from_nodog (RENEW)
	#  8 ask_to_LDAP
	#  9 login_request (req without ctx)
	# 10 load_policy (policy not set or policy without file)
	# ------------------------------------------------------------

	EXIT_VALUE=$1

	unset BACK_TAG

	case "$1" in
	10)
		logger -p $LOCAL_SYSLOG_SELECTOR "$PORTAL_NAME: $REQUEST_URI: load_policy() failure (anomalia 010) POLICY=$POLICY"

		message_page "$SERVICE" "$SERVICE (anomalia 010). Contattare l'assistenza: $TELEFONO"
	;;

	9)
		logger -p $LOCAL_SYSLOG_SELECTOR "$PORTAL_NAME: $REQUEST_URI: login_request() failure (anomalia 009)"

		MSG=`printf "$MSG_ANOMALIA" 009`
		BACK_TAG="<a class=\"back\" href=\"$REDIRECT_DEFAULT\">RIPROVA</a>"

		message_page "$SERVICE" "$MSG"
	;;
	8)
		logger -p $LOCAL_SYSLOG_SELECTOR "$PORTAL_NAME: $REQUEST_URI: Servizio LDAP non disponibile (anomalia 008)"

		if [ "$HTTPS" = "on" ]; then
			MSG="Servizio LDAP non disponibile (anomalia 008). Contattare l'assistenza: $TELEFONO"
		else
			MSG=`printf "$MSG_ANOMALIA" 008`
			BACK_TAG="<a class=\"back\" href=\"$REDIRECT_DEFAULT\">RIPROVA</a>"
		fi

		message_page "Servizio LDAP non disponibile" "$MSG"
	;;

	7)
		logger -p $LOCAL_SYSLOG_SELECTOR "$PORTAL_NAME: $REQUEST_URI: info_notified_from_nodog() failure (anomalia 007)"

		ask_nodog_to_logout_user

		return
	;;
	6)
		logger -p $LOCAL_SYSLOG_SELECTOR "$PORTAL_NAME: $REQUEST_URI: info_notified_from_nodog() failure (anomalia 006) file_ctx=$FILE_CTX"

		ask_nodog_to_logout_user

		return
	;;

	5)
		logger -p $LOCAL_SYSLOG_SELECTOR "$PORTAL_NAME: $REQUEST_URI: send_request_to_nodog() failure (anomalia 005) gateway=$GATEWAY"

		message_page "$SERVICE" "$SERVICE (anomalia 005). Contattare l'assistenza: $TELEFONO"
	;;

	4)
		logger -p $LOCAL_SYSLOG_SELECTOR "$PORTAL_NAME: $REQUEST_URI: sign_data() failure (anomalia 004)"

		MSG=`printf "$MSG_ANOMALIA" 00$1`
		BACK_TAG="<a class=\"back\" href=\"$REDIRECT_DEFAULT\">RIPROVA</a>"

		message_page "$SERVICE" "$MSG"
	;;

	*)
		chmod 777 $HOME/ANOMALIA

		message_page "$SERVICE" "$SERVICE per anomalia interna. Contattare l'assistenza: $TELEFONO"
	;;
	esac

	uscita
}

main_page() {

	# 1 -> mac
	# 2 -> ip
	# 3 -> redirect
	# 4 -> gateway
	# 5 -> timeout
	# 6 -> token
	# 7 -> ap

	if [ -n "$7" -a -n "$4" ]; then
		update_ap_list "$7" "$4"
	fi

	if [ "$VIRTUAL_HOST" = "www.auth-ponza.com" ]; then

		TITLE_TXT="PONZA T-Services"

		HEAD_HTML="<link type=\"text/css\" href=\"css/style.css\" rel=\"stylesheet\">
					  <script type=\"text/javascript\" src=\"js/curvycorners.js\"></script>
					  <script type=\"text/javascript\" src=\"js/effetti.js\"></script>"

		FMT=`cat $DIR_TEMPLATE/login.tmpl`

		BODY_SHTML=`printf "$FMT" $LOGIN_URL "$1" "$2" "$3" "$4" "$5" "$6" "$7" 2>/dev/null`

	elif [ "$VIRTUAL_HOST" = "www.auth-firenze.com" ]; then

		if [ "$REQUEST_URI" = "/login" ]; then

			if [ -r $DIR_TEMPLATE/main.tmpl ]; then

				BODY_SHTML=`cat $DIR_TEMPLATE/main.tmpl 2>/dev/null`

				BODY_SHTML=`printf "$BODY_SHTML" "http://$HTTP_HOST/main?$QUERY_STRING" 2>/dev/null`

				return
			fi

		elif [ "$REQUEST_URI" = "/main" ]; then

			REQUEST_URI=/login
		fi

		get_user_context_connection

		if [ -n "$GATEWAY" ]; then

			# check if he is still connected...

			ask_nodog_status_user

			echo "$OUTPUT" | grep PERMIT >/dev/null 2>&1

			if [ $? -ne 0 ]; then

				unset UUID

				rm -f $FILE_CTX # NB: FILE_CTX e' settato da get_user_context_connection()...
			fi
		fi

		if [ -z "$UUID" ]; then
			# ------------------------------------------------------------------------------------------------------------------------------------------------
			PARAM="$7 $4 $1 $2 $3 $5 $6"
			# ------------------------------------------------------------------------------------------------------------------------------------------------
			# 1 -> ap
			# 2 -> gateway
			# 3 -> mac
			# 4 -> ip
			# 5 -> redirect
			# 6 -> timeout
			# 7 -> token
			# 8 -> uuid
			# ------------------------------------------------------------------------------------------------------------------------------------------------
			# stefano 10.30.1.131:5280 00:e0:4c:d4:63:f5 10.30.1.105 http://www.google.com 0 10.30.1.105&1257603166&2a2436611f452f8eebddce4992e88f8d 055340773
			# ------------------------------------------------------------------------------------------------------------------------------------------------
			if [ "$PARAM" != "      " ]; then
				save_connection_request "$PARAM" # save nodog data on file
			fi
		fi

		ENABLE_BUTTON=a
		DISABLE_BUTTON=del
		CONNECTION_CLOSE=1

		if [ -n "$UUID" ]; then
			# connesso
			REGISTRATI=$DISABLE_BUTTON
			LOGIN=$DISABLE_BUTTON
			LOGOUT=$ENABLE_BUTTON
			STATUS=$ENABLE_BUTTON
		else
			# non connesso
			REGISTRATI=$ENABLE_BUTTON
			LOGIN=$ENABLE_BUTTON
			LOGOUT=$DISABLE_BUTTON
			STATUS=$DISABLE_BUTTON

			UUID_TO_APPEND=1

			user_has_valid_MAC "$@"

			if [ -n "$SSL_CLIENT_CERT_SERIAL" ]; then
				user_has_valid_cert "$@"
			fi
		fi

		CA_CERT_URL="$HTTP_HOST/CA/ServerCA.cer"

		if [ -n "$BROWSER_MSIE" ]; then
			CA_CERT_URL="$HTTP_HOST/CA/ServerCA.der"
		fi

		if [ "$REQUEST_URI" = "/personale" ]; then
			HEAD_HTML="$HEAD_HTML<link type=\"text/css\" href=\"css/button.css\" rel=\"stylesheet\">"
		elif [ "$REQUEST_URI" = "/login" ]; then
			HEAD_HTML="<link type=\"text/css\" href=\"css/firenze.css\" rel=\"stylesheet\">"
		fi

		# post
		# data form hidden
		# certficato CA
		# registrazione
		# bottoni 1-2-3-4

		print_page $LOGIN_URL \
					  "$1" "$2" "$3" "$4" "$5" "$6" "$7" \
					  $CA_CERT_URL \
					  $REGISTRAZIONE_URL \
					  $REGISTRATI $REGISTRATI \
					  $LOGIN		  $LOGIN \
					  $LOGOUT	  $LOGOUT \
					  $STATUS	  $STATUS
	fi
}

get_user_context_connection() {

	# $1 -> uid

	if [ $# -eq 1 ]; then
		FILE_CTX=$DIR_CTX/$1.ctx
	else
		FILE_CTX=`grep -l $REMOTE_ADDR $DIR_CTX/*.ctx 2>/dev/null`
	fi

	# -------------------------------------
	# data connection context saved on file
	# -------------------------------------
	# ap uid gateway mac ip
	# -------------------------------------

	if [ -n "$FILE_CTX" -a -s "$FILE_CTX" ]; then
		read AP UUID GATEWAY MAC IP < $FILE_CTX 2>/dev/null
	else
		unset FILE_CTX AP UUID GATEWAY MAC IP
	fi
}

send_ticket_to_nocat() {

	# ------------------------
	# 1 -> mac
	# 2 -> ip
	# 3 -> redirect
	# 4 -> gateway
	# 5 -> timeout
	# 6 -> token
	# 7 -> ap
	# ------------------------
	# 00:e0:4c:d4:63:f5 10.30.1.105 http://www.google.com 10.30.1.131:5280 stefano 86400 lOosGl9h1aHxo lab2.wpp54
	# ------------------------

	SIGNED_DATA_FILE=/tmp/signed.$$
 	GPG_CMD="gpg --homedir=$HOME/gpg --keyring=$HOME/gpg/trustdb.gpg --sign --armor --no-tty"

	# ----------------------------------------------------------
	# NoCat auth message format (must be PGP signed & encrypted) 
	# ----------------------------------------------------------
	# Action   Permit|Deny
	# Mode     login|??? (popup/renew?)
	# Redirect URL_requested
	# Mac      00:11:22:33:44:55:66
	# Timeout  to_in_seconds
	# Token    token
	# ----------------------------------------------------------

	$GPG_CMD <<END >$SIGNED_DATA_FILE 2>/dev/null

Action   Permit
Mode	   Login
Redirect	http://$HTTP_HOST/postlogin?gateway=$4&redirect=$3&ap=$7&ip=$2&mac=$1&timeout=$5
Mac		$1
Timeout	$5
Token		$6
END

	if [ -s $SIGNED_DATA_FILE ]; then

		SIGNED_DATA=`tail -n +4 $SIGNED_DATA_FILE | head -n -1 | tr -d '\r\n' 2>/dev/null`

	 	rm -f $SIGNED_DATA_FILE
	fi

	HTTP_RESPONSE_BODY="<html><body>OK</body></html>"
	HTTP_RESPONSE_HEADER="Refresh: 0; url=http://$4/?ticket=$SIGNED_DATA\r\n"
}

send_ticket_to_nodog() {

	# ------------------------
	# $1 -> mac
	# $2 -> ip
	# $3 -> redirect
	# $4 -> gateway
	# $5 -> timeout
	# $6 -> token
	# $7 -> ap
	# $8 -> uid
	# ------------------------

	# -------------------------------------------------------------
	# CHECK FOR CHANGE OF CONNECTION CONTEXT FOR SAME USER ID
	# -------------------------------------------------------------
	check_if_user_is_connected "$1" "$2" "$4" "$7" "$8"

	if [ "$OP" = "RENEW" ]; then
		ask_nodog_to_logout_user
	fi

	# --------------------------------------------------------------------------
	# SAVE REAL UID AND POLICY ON FILE (UUID_TO_LOG POLICY MAX_TIME MAX_TRAFFIC)
	# --------------------------------------------------------------------------
	FILE_UID=$DIR_REQ/$8.uid

	if [ -z "$WA_UID" ]; then
		WA_UID=$8
	fi

	write_FILE "$WA_UID $POLICY $MAX_TIME $MAX_TRAFFIC" $FILE_UID
	# --------------------------------------------------------------------

	FILE_CNT=$DIR_CNT/$POLICY/$WA_UID
	# --------------------------------------------------------------------
	# TIME POLICY
	# --------------------------------------------------------------------
	if [ -z "$MAX_TIME" ]; then
		MAX_TIME=0
	fi

	if [ $MAX_TIME -gt 0 ]; then
		# --------------------------------------------------------------------
		# WE CHECK FOR THE TIME REMAIN FOR CONNECTION (SECS) SAVED ON FILE
		# --------------------------------------------------------------------
		REMAIN=$FILE_CNT.timeout

		if [ ! -s $REMAIN ]; then

			TIMEOUT=$MAX_TIME
			# ---------------------------------------------------------
			# we save the time remain for connection (secs) on file
			# ---------------------------------------------------------
			write_FILE $TIMEOUT $FILE_CNT.timeout
			# ---------------------------------------------------------
		else
			read TIMEOUT < $REMAIN 2>/dev/null

			if [ $TIMEOUT -eq 0 ]; then

				check_if_user_connected_to_AP_NO_CONSUME "$7"

				if [ "CONSUME_ON" = "true" ]; then
					message_page "Tempo consumato" "Hai consumato il tempo disponibile del servizio!"
				fi
			fi
		fi
		# --------------------------------------------------------------------
	fi
	# --------------------------------------------------------------------

	# --------------------------------------------------------------------
	# TRAFFIC POLICY
	# --------------------------------------------------------------------
	if [ -z "$MAX_TRAFFIC" ]; then
		MAX_TRAFFIC=0
	fi

	if [ $MAX_TRAFFIC -gt 0 ]; then
		# --------------------------------------------------------------------
		# WE CHECK FOR THE TRAFFIC REMAIN FOR CONNECTION (BYTES) SAVED ON FILE
		# --------------------------------------------------------------------
		REMAIN=$FILE_CNT.traffic

		if [ ! -s $REMAIN ]; then

			TRAFFIC=$MAX_TRAFFIC
			# ---------------------------------------------------------
			# we save the remain traffic for connection (bytes) on file
			# ---------------------------------------------------------
			write_FILE $TRAFFIC $FILE_CNT.traffic
			# ---------------------------------------------------------
		else
			read TRAFFIC < $REMAIN 2>/dev/null

			if [ $TRAFFIC -eq 0 ]; then

				check_if_user_connected_to_AP_NO_CONSUME "$7"

				if [ "CONSUME_ON" = "true" ]; then
					message_page "Traffico consumato" "Hai consumato il traffico disponibile del servizio!"
				fi
			fi
		fi
		# --------------------------------------------------------------------
	fi
	# --------------------------------------------------------------------

	if [ -z "$REDIRECT_DEFAULT" ]; then
		REDIRECT_DEFAULT=$3
	fi

	SIGNED_DATA="
Action   Permit
Mode	   Login
Redirect	http://$HTTP_HOST/postlogin?uid=$8&gateway=$4&redirect=$REDIRECT_DEFAULT&ap=$7&ip=$2&mac=$1&timeout=$TIMEOUT&traffic=$TRAFFIC
Mac		$1
Timeout	$TIMEOUT
Traffic	$TRAFFIC
Token		$6
User		$8"

	sign_data

	write_to_LOG "$8" "$7" "$2" "$1" "$TIMEOUT" "$TRAFFIC"

	if [ -n "$UUID_TO_APPEND" ]; then
		append_to_FILE " $8" $REQ_FILE # NB: si aggiunge UUID alla request...
	fi

	HTTP_RESPONSE_BODY="<html><body>OK</body></html>"
	HTTP_RESPONSE_HEADER="Refresh: 0; url=http://$4/ticket?ticket=$SIGNED_DATA\r\n"
}

send_ticket() {

#	send_ticket_to_nocat "$@" 
	send_ticket_to_nodog "$@" 

	write_SSI
}

user_has_valid_MAC() {

	# ------------------------
	# 1 -> mac
	# 2 -> ip
	# 3 -> redirect
	# 4 -> gateway
	# 5 -> timeout
	# 6 -> token
	# 7 -> ap
	# ------------------------
	# 00:e0:4c:d4:63:f5 10.30.1.105 http://www.google.com 10.30.1.131:5280 stefano 86400 lOosGl9h1aHxo lab2.wpp54
	# ------------------------

	# List of allowed MAC

	FILE=$HOME/etc/$VIRTUAL_HOST/.MAC_WHITE_LIST

	if [ -s $FILE ]; then

		load_policy

		while read MAC
		do
			if [ "$MAC" = "$1" ]; then

				# ap is calling for auth, redirect back to the gateway appending a signed ticket that will signal ap to unlock the firewall...

				OP=MAC_AUTH

				send_ticket "$@" "$MAC" 
			fi
		done < $FILE
	fi
}

check_if_user_connected_to_AP_NO_CONSUME() {

	# List of Access Point with NO CONSUME

	FILE=$HOME/etc/$VIRTUAL_HOST/.AP_NO_CONSUME

	if [ -s $FILE ]; then

		while read AP_WITH_NO_COMSUME
		do
			if [ "AP_WITH_NO_COMSUME" = "$1" ]; then

				unset CONSUME_ON

				return
			fi
		done < $FILE
	fi

	CONSUME_ON=true
}

_date() { date '+%Y/%m/%d %H:%M:%S' ; }

write_to_LOG() {

	# $1 -> uid
	# $2 -> ap
	# $3 -> ip
	# $4 -> mac
	# $5 -> timeout
	# $6 -> traffic

	# --------------------------------------------------------------------
	# GET REAL UID FROM FILE (UUID_TO_LOG POLICY MAX_TIME MAX_TRAFFIC)
	# --------------------------------------------------------------------
	FILE_UID=$DIR_REQ/$1.uid

	read UUID_TO_LOG POLICY < $FILE_UID 2>/dev/null

	if [ -z "$UUID_TO_LOG" ]; then
		UUID_TO_LOG=$1
	fi
	# --------------------------------------------------------------------

	RIGA="`_date` op: $OP, uid: $UUID_TO_LOG, ap: $2, ip: $3, mac: $4, timeout: $5, traffic: $6 policy: $POLICY"

	append_to_FILE "$RIGA" $FILE_LOG

	sync

	logger -p $REMOTE_SYSLOG_SELECTOR "$PORTAL_NAME: $RIGA"
}

send_request_to_nodog() {

	# $1 -> request
	# $2 -> filename to save output
	# $3 -> option

	ACCESS_POINT_NAME=${AP##*@}

	ACCESS_POINT=`grep "^$ACCESS_POINT_NAME " $ACCESS_POINT_LIST.down 2>/dev/null`

	if [ -z "$ACCESS_POINT" ]; then

		ACCESS_POINT=`grep "^$ACCESS_POINT_NAME " $ACCESS_POINT_LIST.up 2>/dev/null`

		if [ -n "$ACCESS_POINT" ]; then

			GATEWAY=`echo -n $ACCESS_POINT | cut -d' ' -f2 2>/dev/null`

			# -----------------------------------------------------------------------------
			# we send request to nodog
			# -----------------------------------------------------------------------------
			# NB: we need PREFORK_CHILD > 2
			# -----------------------------------------------------------------------------
			# UTRACE="0 10M 0"
			# UOBJDUMP="0 100k 10"
			# USIMERR="error.sim"
			# export UTRACE UOBJDUMP USIMERR
			# -----------------------------------------------------------------------------

			OUTPUT=`$CLIENT_HTTP $3 http://$GATEWAY/$1 2>>/tmp/CLIENT_HTTP.err`

			if [ $? -ne 0 ]; then

				# si aggiunge access point alla lista di quelli non contattabili...

				append_to_FILE "$ACCESS_POINT_NAME $GATEWAY" $ACCESS_POINT_LIST.down

				anomalia 5
			fi

			if [ -n "$2" -a \
				  -n "$OUTPUT" ]; then

				write_FILE "$OUTPUT" $2

				unset OUTPUT
			fi
		fi
	fi
}

# NB: check if /etc/openldap/ldap.conf contains TLS_REQCERT never
# --------------------------------------------------------------------------------------------------------------------
# ldapsearch -v -H 'ldaps://94.138.39.149:636' -b ou=users,o=unwired-portal -D cn=admin,o=unwired-portal -w programmer

ask_to_LDAP() {

	# $1 -> cmd
	# $2 -> param
	# $3 -> option
	# $4 -> filter_option

	TMPFILE=/tmp/ask_to_LDAP.$$

	if [ "$1" = ldapsearch ]; then
		ldapsearch $2 "$3" $4 >$TMPFILE.out 2>$TMPFILE.err
	else
				  $1 $2 <<END   >$TMPFILE.out 2>$TMPFILE.err
$3
END
	fi

	EXIT_VALUE=$?

	if [ $EXIT_VALUE -eq 0 ]; then

		rm -f $TMPFILE.err

	elif [ -s $TMPFILE.err ]; then

		rm -f $TMPFILE.out

		grep '(-1)' < $TMPFILE.err # Can't contact LDAP server (-1)

		if [ $? -eq 0 ]; then
			anomalia 8
		fi
	fi
}

load_policy() {

	USER_MAX_TIME=0
	USER_MAX_TRAFFIC=0

	if [ -n "$1" ]; then
		# --------------------------------------------------------------------
		# GET POLICY FROM FILE (UUID_TO_LOG POLICY MAX_TIME MAX_TRAFFIC)
		# --------------------------------------------------------------------
		read UUID_TO_LOG POLICY USER_MAX_TIME USER_MAX_TRAFFIC < $1 2>/dev/null
	fi

	if [ -z "$POLICY" ]; then
		anomalia 10
	fi

	POLICY_FILE=$DIR_POLICY/$POLICY

	if [ ! -s $POLICY_FILE ]; then
		anomalia 10
	fi

 	mkdir -p $DIR_CNT/$POLICY

	unset POLICY_RESET
	unset BONUS_FOR_EXIT

	source $DIR_POLICY/$POLICY
	# --------------------------------------------------------------------
	# TIME POLICY
	# --------------------------------------------------------------------
	if [ -z "$MAX_TIME" ]; then
		MAX_TIME=0
	fi
	if [ $USER_MAX_TIME -ne $MAX_TIME ]; then
		MAX_TIME=$USER_MAX_TIME
	fi
	# --------------------------------------------------------------------
	# TRAFFIC POLICY
	# --------------------------------------------------------------------
	if [ -z "$MAX_TRAFFIC" ]; then
		MAX_TRAFFIC=0
	fi
	if [ $USER_MAX_TRAFFIC -ne $MAX_TRAFFIC ]; then
		MAX_TRAFFIC=$USER_MAX_TRAFFIC
	fi
	# --------------------------------------------------------------------
}

save_connection_request() {

	# $1 -> data

	# ---------------------------------------------------------------------------------
	# SAVE REQUEST CONTEXT DATA ON FILE (AP GATEWAY MAC IP REDIRECT TIMEOUT TOKEN UUID)
	# ---------------------------------------------------------------------------------
	REQ_FILE=$DIR_REQ/$SESSION_ID.req

	write_FILE "$1" $DIR_REQ/$SESSION_ID.req "-n"
	# ---------------------------------------------------------------------------------
}

save_connection_context() {

	# $1 -> ap
	# $2 -> uid
	# $3 -> gateway
	# $4 -> mac
	# $5 -> ip

	# --------------------------------------------------------------------
	# SAVE CONNECTION CONTEXT DATA ON FILE (AP UUID GATEWAY MAC IP)
	# --------------------------------------------------------------------
	FILE_CTX=$DIR_CTX/$2.ctx

	write_FILE "$1 $2 $3 $4 $5" $FILE_CTX
	# --------------------------------------------------------------------
}

get_user_context_connection() {

	# $1 -> uid

	if [ $# -eq 1 ]; then
		FILE_CTX=$DIR_CTX/$1.ctx
	else
		FILE_CTX=`grep -l $REMOTE_ADDR $DIR_CTX/*.ctx 2>/dev/null`
	fi

	# data connection context saved on file
	# -------------------------------------
	# ap uid gateway mac ip

	if [ -n "$FILE_CTX" -a -s "$FILE_CTX" ]; then
		read AP UUID GATEWAY MAC IP < $FILE_CTX 2>/dev/null
	else
		unset FILE_CTX AP UUID GATEWAY MAC IP
	fi
}

check_if_user_is_connected() {

	# $1 -> mac
	# $2 -> ip
	# $3 -> gateway
	# $4 -> ap
	# $5 -> uid

	get_user_context_connection $5

	if [ -n "$FILE_CTX" ]; then

		if [ "$MAC"		 != "$1" -o \
			  "$IP"		 != "$2" -o \
			  "$GATEWAY" != "$3" -o \
			  "$AP"		 != "$4" ]; then

			OP=RENEW
		fi
	fi
}

user_has_valid_cert() {

 	ask_to_LDAP ldapsearch "-LLL -b $WIAUTH_USER_BASEDN $LDAP_USER_PARAM" \
			"(&(objectClass=waUser)(&(waIssuer=$SSL_CLIENT_I_DN)(waSerial=$SSL_CLIENT_CERT_SERIAL)(waActive=TRUE)))"

	if [ -s $TMPFILE.out ]; then

		USER=`cat $TMPFILE.out | grep 'waUid: ' | cut -f2 -d' ' 2>/dev/null`

		if [ -z "$USER" ]; then
			USER=unknow
		fi

		# NoDog is calling for auth, redirect back to the gateway appending a signed ticket that will signal NoDog to unlock the firewall...

		OP=CERT_AUTH

		send_ticket "$@" "$USER"
	fi
}

login_with_problem() {

	BACK_TAG="<a class=\"back\" href=\"$REDIRECT_DEFAULT\">RIPROVA</a>"

	message_page "Login" "Problema in fase di autenticazione. Si prega di riprovare, se il problema persiste contattare: $TELEFONO"
}

logout_with_problem() {

	unset BACK_TAG

	message_page "ID di sessione mancante" "Utente non connesso (session id: $SESSION_ID)"
}

ask_nodog_status_user() {

	# we request the status of the indicated user...
	# -----------------------------------------------------------------------------
	# NB: we need PREFORK_CHILD > 2
	# -----------------------------------------------------------------------------
	send_request_to_nodog "status?ip=$REMOTE_ADDR"
}

ask_nodog_to_logout_user() {

	# we request to logout this user with the old ip from the associated gateway...
	# -----------------------------------------------------------------------------
	# NB: we need PREFORK_CHILD > 2
	# -----------------------------------------------------------------------------
	SIGNED_DATA="ip=$IP&mac=$MAC"

	sign_data

	send_request_to_nodog "logout?$SIGNED_DATA"
}

sign_data() {

	SIGNED_DATA=`echo -n -E "$SIGNED_DATA" | openssl des3 -pass pass:200912281747 -a -e | tr -d '\n'`
}

info_notified_from_nodog() {

	# $1 -> mac
	# $2 -> ip
	# $3 -> gateway
	# $4 -> ap
	# $5 -> uid
	# $6 -> logout
	# $7 -> connected
	# $8 -> traffic

	OP=LOGOUT

	unset LOGOUT

	append_to_FILE "`_date` op: INFO uid: $5, ap: $4, ip: $2, mac: $1, logout: $6, connected: $7, traffic: $8" $FILE_LOG.info

	if [ -z "$5" ]; then
		return
	fi

	# NB: FILE_CTX e' settato da get_user_context_connection() che e' chiamato da check_if_user_is_connected()...

	check_if_user_is_connected "$1" "$2" "$3" "$4" "$5"

	if [ -z "$FILE_CTX" -o \
		  "$OP" = "RENEW" ]; then

		IP=$2
		MAC=$1
		GATEWAY=$3

		if [ "$OP" = "RENEW" ]; then
			anomalia 7
		else
			save_connection_context $4 $5 $3 $1 $2 # save connection context data on file (ap uuid gateway mac ip) to avoid another anomalia...

			anomalia 6
		fi
	fi

	check_if_user_connected_to_AP_NO_CONSUME "$4"

	if [ "$CONSUME_ON" = "true" ]; then

		load_policy $DIR_REQ/$5.uid # GET POLICY FROM FILE (UUID_TO_LOG POLICY MAX_TIME MAX_TRAFFIC)

		FILE_CNT=$DIR_CNT/$POLICY/$UUID_TO_LOG

		# --------------------------------------------------------------------
		# TRAFFIC POLICY
		# --------------------------------------------------------------------
		if [ -z "$MAX_TRAFFIC" ]; then
			MAX_TRAFFIC=0
		fi

		if [ $MAX_TRAFFIC -gt 0 ]; then
			# --------------------------------------------------------------------
			# WE CHECK FOR THE TRAFFIC REMAIN FOR CONNECTION (BYTES) SAVED ON FILE
			# --------------------------------------------------------------------
			TRAFFIC=0

			if [ -s $FILE_CNT.traffic ]; then
				read TRAFFIC < $FILE_CNT.traffic 2>/dev/null

				let "TRAFFIC = TRAFFIC - $8"

				if [ $TRAFFIC -lt 0 ]; then
					TRAFFIC=0
				fi
			fi
			# ---------------------------------------------------------
			# we save the remain traffic for connection (bytes) on file
			# ---------------------------------------------------------
			write_FILE $TRAFFIC $FILE_CNT.traffic

			if [ $TRAFFIC -eq 0 -a \
				  -z "$LOGOUT" ]; then

				LOGOUT=true

				ask_nodog_to_logout_user
			fi
			# ---------------------------------------------------------
		fi
		# ---------------------------------------------------------

		# --------------------------------------------------------------------
		# TIME POLICY
		# --------------------------------------------------------------------
		if [ -z "$MAX_TIME" ]; then
			MAX_TIME=0
		fi

		if [ $MAX_TIME -gt 0 ]; then
			# --------------------------------------------------------------------
			# WE CHECK FOR THE TIME REMAIN FOR CONNECTION (SECS) SAVED ON FILE
			# --------------------------------------------------------------------
			TIMEOUT=0

			if [ -s $FILE_CNT.timeout ]; then

				read TIMEOUT < $FILE_CNT.timeout 2>/dev/null

				let "TIMEOUT = TIMEOUT - $7"

				if [ -n "$BONUS_FOR_EXIT" -a $6 -eq -1 ]; then # disconneted (logout implicito)
					let "TIMEOUT = TIMEOUT + $BONUS_FOR_EXIT"
				fi

				if [ $TIMEOUT -lt 0 ]; then
					TIMEOUT=0
				fi
			fi
			# ---------------------------------------------------------
			# we save the time remain for connection (secs) on file
			# ---------------------------------------------------------
			write_FILE $TIMEOUT $FILE_CNT.timeout

			if [ $TIMEOUT -eq 0 -a \
				  -z "$LOGOUT" ]; then

				LOGOUT=true

				ask_nodog_to_logout_user
			fi
			# ---------------------------------------------------------
		fi
		# ---------------------------------------------------------
	fi

	if [ $6 -ne 0 ]; then # logout

		LOGOUT=true

		if [ $6 -eq -1 ]; then # disconneted (logout implicito)
			OP=EXIT
		fi

		write_to_LOG "$5" "$4" "$2" "$1" "$TIMEOUT" "$TRAFFIC"

		rm -f $FILE_CTX $DIR_REQ/$2.req $DIR_REQ/$5.uid # we remove the data saved on file (connection context data and NoDog data)
	fi

	if [ $# -gt 8 ]; then

		shift 8

		info_notified_from_nodog "$@"

	elif [ -n "$LOGOUT" ]; then
		BODY_SHTML="LOGOUT"
	else
		BODY_SHTML="OK"
	fi
}

ask_nodog_to_check_for_users_info() {

	# we request nodog to check for users logout or disconnect...
	# -----------------------------------------------------------------------------
	# NB: we need PREFORK_CHILD > 2
	# -----------------------------------------------------------------------------
	send_request_to_nodog "check" $FILE_TMP "-i"

	if [ -s $FILE_TMP ]; then

		read HTTP_VERSION HTTP_STATUS HTTP_DESCR < $FILE_TMP 2>/dev/null

		if [ "$HTTP_STATUS" = "204" ]; then # 204 - HTTP_NO_CONTENT

			sleep 9

			ask_nodog_to_check_for_users_info
		fi
	fi
}

load_value_session() {

	if [ -s $TMP_FORM_FILE ]; then

		i=1
		while read LINE
		do
			eval v$i=\"$LINE\"

			let "i = i + 1"
		done < $TMP_FORM_FILE
	fi
}

get_user_nome_cognome() {

	TMP_FORM_FILE=$DIR_REG/$UUID.reg

	load_value_session

	if [ -n "$v1$v2" ]; then
		USER="$v1 $v2"
	else
		USER=$UUID
	fi
}

logout() {

	REQ_FILE=$DIR_REQ/$SESSION_ID.req # nodog data saved on file

	if [ ! -s $REQ_FILE ]; then
		logout_with_problem
	fi

	read AP GATEWAY MAC IP REDIRECT TIMEOUT TOKEN UUID < $REQ_FILE 2>/dev/null

	if [	 -z "$UUID" -o \
		  ! -s $DIR_CTX/$UUID.ctx ]; then
		unset BACK_TAG

		message_page "Utente non connesso" "Utente non connesso"
	fi

	# --------------------------------------------------------------------
	# GET POLICY FROM FILE (UUID_TO_LOG POLICY MAX_TIME MAX_TRAFFIC)
	# --------------------------------------------------------------------
	load_policy $DIR_REQ/$UUID.uid

	ask_nodog_to_logout_user
}

logout_request() {

	logout

	PARAM=""

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

	REQUEST_URI=ringraziamenti

	print_page "$PARAM"
}

logout_notified_from_nodog() {

	TMPFILE=/tmp/info_notified_from_nodog.$$

	echo "$@" > $TMPFILE

	info_notified_from_nodog "$@"

	rm -f $TMPFILE
}

get_timeout_secs() {

	DSTART=`printf "%4s-%2s-%2s %2s:%2s:%2s" ${1:0:4} ${1:4:2} ${1:6:2} ${1:8:2} ${1:10:2} ${1:12:2} 2>/dev/null`
	  DEND=`printf "%4s-%2s-%2s %2s:%2s:%2s" ${2:0:4} ${2:4:2} ${2:6:2} ${2:8:2} ${2:10:2} ${2:12:2} 2>/dev/null`

	START=`date --date="$DSTART" +%s 2>/dev/null`
	  END=`date --date="$DEND"   +%s 2>/dev/null`

	let "TIMEOUT = $END - $START"
}

login_request() {

	# $1	-> mac
	# $2  -> ip
	# $3	-> redirect
	# $4	-> gateway
	# $5	-> timeout
	# $6	-> token
	# $7	-> ap
	# $8  -> realm (10_piazze, paas, ...)
	# $9  -> uid
	# $10 -> password
	# $11 -> bottone

	REQ_FILE=$DIR_REQ/$SESSION_ID.req # nodog data saved on file

	if [ ! -s $REQ_FILE ]; then
		login_with_problem
	fi

	read AP GATEWAY MAC IP REDIRECT TIMEOUT TOKEN UUID < $REQ_FILE 2>/dev/null

	if [ -n "$UUID" ]; then
		FILE_CTX=$DIR_CTX/$UUID.ctx

		if [ ! -s $FILE_CTX ]; then
			anomalia 9
		fi

		unset BACK_TAG

		message_page "Login" "Sei già loggato! (login_request)"
	fi

	if [ -n "$7" -a "$7" != "$AP" ]; then
		login_with_problem
	fi
	if [ -n "$4" -a "$4" != "$GATEWAY" ]; then
		login_with_problem
	fi
	if [ -n "$1" -a "$1" != "$MAC" ]; then
		login_with_problem
	fi
	if [ -n "$2" -a "$2" != "$IP" ]; then
		login_with_problem
	fi
	if [ -n "$5" -a "$5" != "$TIMEOUT" ]; then
		login_with_problem
	fi
	if [ -n "$3" -a "$3" != "$REDIRECT" ]; then
		login_with_problem
	fi
	if [ -n "$6" -a "$6" != "$TOKEN" ]; then
		login_with_problem
	fi

	UUID_TO_APPEND=1

	if [ "$8" = "paas" ]; then

		OP=AUTH_PAAS

		# TODO:
		# ---------------------------------------------------------------------------------
		# ask_to_LDAP ldapsearch ...
		# ---------------------------------------------------------------------------------

		if [ ! -s $TMPFILE.out ]; then
			unset BACK_TAG

			message_page "Errore" "Errore Autorizzazione dominio PASS"
		fi

	elif [ "$8" = "10_piazze" ]; then

		OP=PASS_AUTH

		if [ -z "$9" -o -z "${10}" ]; then
			unset BACK_TAG

			message_page "Impostare utente e/o password" "Impostare utente e/o password"
		fi

		# Check 1: Wrong user and/or password

		ask_to_LDAP ldapsearch "-LLL -b $WIAUTH_CARD_BASEDN $LDAP_CARD_PARAM" "(&(waLogin=$9)(waPassword=${10}))"

		if [ ! -s $TMPFILE.out ]; then
			unset BACK_TAG

			message_page "Utente e/o Password errato/i" "Credenziali errate!"
		fi

		# --------------------------------------------------------------------
		# GET USER FOR THIS CARD
		# --------------------------------------------------------------------
		WA_UID=`grep 'waUsedBy: ' $TMPFILE.out | cut -f2 -d' ' 2>/dev/null`

		# Check 2: Activation required

		if [ -z "$WA_UID" ]; then
			unset BACK_TAG

			message_page "Attivazione non effettuata" "Per utilizzare il servizio e' richiesta l'attivazione"
		fi
		# --------------------------------------------------------------------

		# Check 3: Card revoked

		REVOKED=`grep 'waRevoked: ' $TMPFILE.out | cut -f2 -d' ' 2>/dev/null`

		if [ "$REVOKED" != "FALSE" ]; then
			unset BACK_TAG

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

		# Check 4: Not After

		NOT_AFTER=`grep 'waNotAfter: ' $TMPFILE.out | cut -f2 -d' ' 2>/dev/null`

		if [ -n "$NOT_AFTER" ]; then

			# Check 5: Expired validity

			GEN_TIME=`date +%Y%m%d%H%M%SZ` # GeneralizedTime YYYYmmddHH[MM[SS]][(./,)d...](Z|(+/-)HH[MM])

			get_timeout_secs "$GEN_TIME" "$NOT_AFTER"

			if [ $TIMEOUT -lt 0 ]; then
				unset BACK_TAG

				message_page "Validita' scaduta" "La tua validita' e' scaduta!"
			fi

		else

			OP=FIRST_PASS_AUTH

			# --------------------------------------------------------------------
			# waTime - valore di *** CONSUMO ***
			# --------------------------------------------------------------------
			if [ "$POLICY" != "FLAT" ]; then

				WA_TIME=`grep 'waTime: ' $TMPFILE.out | cut -f2 -d' ' 2>/dev/null`

				if [ -n "$WA_TIME" ]; then
					MAX_TIME=$WA_TIME
				fi
			fi
			# --------------------------------------------------------------------
			# waTraffic - valore di *** CONSUMO ***
			# --------------------------------------------------------------------
			if [ "$POLICY" != "FLAT" ]; then

				WA_TRAFFIC=`grep 'waTraffic: ' $TMPFILE.out | cut -f2 -d' ' 2>/dev/null`

				if [ -n "$WA_TRAFFIC" ]; then
					MAX_TRAFFIC=$WA_TRAFFIC
				fi
			fi
			# --------------------------------------------------------------------

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
	else
		unset BACK_TAG

		message_page "Errore" "Errore Autorizzazione - dominio sconosciuto: $8"
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

	send_ticket "$MAC" $IP "$REDIRECT" $GATEWAY $TIMEOUT "$TOKEN" $AP "$9"
}

postlogin() {

	if [ $# -eq 8 ]; then

		unset BACK_TAG

		# $1 -> uid
		# $2 -> gateway
		# $3 -> redirect
		# $4 -> ap
		# $5 -> ip
		# $6 -> mac
		# $7 -> timeout
		# $8 -> traffic

		REQ_FILE=$DIR_REQ/$SESSION_ID.req

		if [ ! -s $REQ_FILE ]; then
			login_with_problem
		fi

		FILE_CTX=$DIR_CTX/$1.ctx

		if [ -s $FILE_CTX ]; then
			message_page "PostLogin" "Sei già loggato! (postlogin)"
		fi

		# stefano 10.30.1.131:5280 00:e0:4c:d4:63:f5 10.30.1.105 http://www.google.com 0 10.30.1.105&1257603166&2a2436611f452f8eebddce4992e88f8d 055340773

		read AP GATEWAY MAC IP REDIRECT TIMEOUT TOKEN UUID < $REQ_FILE 2>/dev/null

		if [ -z "$UUID" ]; then
			login_with_problem
		fi

		OP=LOGIN

		write_to_LOG "$1" "$4" "$5" "$6" "$7" "$8"

		# --------------------------------------------------------------------
		# SAVE CONNECTION CONTEXT DATA ON FILE (AP UUID GATEWAY MAC IP)
		# --------------------------------------------------------------------
		save_connection_context $4 $1 $2 $6 $5
		# --------------------------------------------------------------------

		CONNECTION_CLOSE=1

		BODY_STYLE=`printf "onload=\"doOnLoad('postlogin?uid=%s&gateway=%s','%s')\"" "$1" "$2" "$3" 2>/dev/null`
 		HEAD_HTML="<script type=\"text/javascript\" src=\"js/logout_popup.js\"></script>"

		print_page "$1" "$3" "$3"

	elif [ $# -eq 2 ]; then

		# $1 -> uid
		# $2 -> gateway

		TITLE_TXT="Logout popup"
		REQUEST_URI=logout_popup

		CONNECTION_CLOSE=1

		print_page "$1" "$1"
	fi
}

logout_notified_from_popup() {

	logout

	CONNECTION_CLOSE=1

	HEAD_HTML="<script type=\"text/javascript\" src=\"js/logout_popup.js\"></script>"
	BODY_STYLE='onload="CloseItAfterSomeTime()"'

	REQUEST_URI=logout_notify

	print_page "$1"
}

registrazione_form() {

	# eventuali dati pre form input

	TMP_FORM_FILE=/tmp/registrazione.prev

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

	TITLE_TXT="Registrazione utente"
	HEAD_HTML="<link type=\"text/css\" href=\"css/style.css\" rel=\"stylesheet\">
				  <script type=\"text/javascript\" src=\"js/livevalidation_standalone.js\"></script>"

	print_page $REGISTRAZIONE_URL \
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
			"iframe/tutela_dati.txt"
}

save_value_session() {

	cat <<END >$TMP_FORM_FILE
$1
$2
$3
$4
$5
$6
$7
$8
$9
${10}
${11}
${12}
${13}
${14}
${15}
${16}
${17}
END
#${18}
#${19}
#${20}
#${21}
#${22}
#${23}
#${24}
}

check_phone_number() {

	# -----------------------------------------
	# $1 -> CALLER_ID
	# -----------------------------------------

	# Check for italian prefix

	CALLER_ID=`echo -n "$1" | \
		awk 'BEGIN {phoneNumber=""}
		{

		if (match($1,"^+") == 0 && match($1,"^00") == 0) {
			phoneNumber="+39"$1	
		} else {
			phoneNumber=$1
		}

		if (match(phoneNumber,"^[0-9]+$") == 0 && match(phoneNumber,"^+[0-9]+$") == 0) {
			print phoneNumber": ""Invalid phone number"; exit (1);
		}

		if (match(phoneNumber,"^+39") == 0 && match(phoneNumber,"^0039") == 0) {
			print phoneNumber": ""Not italian number"; exit (1);
		}

		if (match(phoneNumber,"^[0-9]+$")) {
			print substr(phoneNumber,5);
		}

		if (match(phoneNumber,"^+[0-9]+$")) {
			print substr(phoneNumber,4);
		}

		}' 2>/dev/null`

	EXIT_VALUE=$?
}

registrazione_request() {

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
"

	if [ $EXIT_VALUE -eq 68 ]; then
		message_page "Utente già registrato" "Utente già registrato (ldap branch card)"
	fi

	TITLE_TXT="Registrazione effettuata"
	REQUEST_URI=post_registrazione

	print_page $CALLER_ID "polling_attivazione" $CALLER_ID
}

card_activation() {

	# -----------------------------------------
	# $1 -> CALLER_ID
	# -----------------------------------------

	check_phone_number "$1"

	if [ $EXIT_VALUE -ne 0 ]; then

		logger -p $LOCAL_SYSLOG_SELECTOR "$PORTAL_NAME: card_activation: $CALLER_ID"

		uscita
	fi

	# Search card by pin

	ask_to_LDAP ldapsearch "-LLL -b $WIAUTH_CARD_BASEDN $LDAP_CARD_PARAM" "waPin=$CALLER_ID"

	if [ ! -s $TMPFILE.out ]; then

		logger -p $LOCAL_SYSLOG_SELECTOR "$PORTAL_NAME: card_activation: Utente $CALLER_ID non registrato!"

		uscita
	fi

	# Verify the card is already activated

	WA_USEDBY=`awk '/^waUsedBy/{print $2}' $TMPFILE.out 2>/dev/null`

	if [ -n "$WA_USEDBY" ]; then

		logger -p $LOCAL_SYSLOG_SELECTOR "$PORTAL_NAME: card_activation: Utente $CALLER_ID già attivato!"

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

		logger -p $LOCAL_SYSLOG_SELECTOR "$PORTAL_NAME: card_activation: Update card failed!"

		uscita
	fi

	logger -p $LOCAL_SYSLOG_SELECTOR "$PORTAL_NAME: card_activation: Login    <$CALLER_ID>"
	logger -p $LOCAL_SYSLOG_SELECTOR "$PORTAL_NAME: card_activation: Password <$PASSWORD>"

	BODY_SHTML="OK"
}

execute_recovery() {

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

	rm -f $DIR_CTX/"$1".ctx $DIR_REG/"$1".reg $DIR_CNT/$POLICY/"$1".*

	logger -p $LOCAL_SYSLOG_SELECTOR "$PORTAL_NAME: recovery: User <$1> recovered"

	BACK_TAG="<a href=\"admin\">TORNA AL MENU</a>"

	message_page "Esito recovery" "Recovery completato!"
}

stato_utente() {

	get_user_context_connection

	if [ -z "$GATEWAY" ]; then
		message_page "Utente non connesso" "Utente non connesso"
	else
		get_user_nome_cognome

		ask_nodog_status_user

		TITLE_TXT="Stato utente"

		FMT=`cat $DIR_TEMPLATE/stato_utente.tmpl 2>/dev/null`
		DATE=`date`

		BODY_SHTML=`printf "$FMT" "$DATE" $AP "$OUTPUT" "$BACK_TAG" 2>/dev/null`
	fi
}

view_user() {

	# $1 -> uid

	ask_to_LDAP ldapsearch "-LLL -b $WIAUTH_USER_BASEDN $LDAP_USER_PARAM" "waUid=$1"

	if [ ! -s $TMPFILE.out ]; then
		message_page "Visualizzazione dati utente: utente non registrato" "Visualizzazione dati utente: $1 non registrato!"
	fi

	WA_ACTIVE=`awk '/^waActive/{print $2}' $TMPFILE.out 2>/dev/null`
	WA_UID=`	  awk	'/^waUid/{print $2}'		$TMPFILE.out 2>/dev/null`
	WA_CELL=`  awk	'/^waCell/{print $2}'	$TMPFILE.out 2>/dev/null`

	ask_to_LDAP ldapsearch "-LLL -b $WIAUTH_CARD_BASEDN $LDAP_CARD_PARAM" "waUsedBy=$1"

	if [ ! -s $TMPFILE.out ]; then
		message_page "Visualizzazione dati utente: utente non attivato" "Visualizzazione dati utente: $1 non attivato!"
	fi

	WA_CID=`		 awk	'/^waCid/{print $2}'			$TMPFILE.out 2>/dev/null`
	WA_PIN=`		 awk	'/^waPin/{print $2}'			$TMPFILE.out 2>/dev/null`
	WA_CARDID=`	 awk	'/^waCardId/{print $2}'		$TMPFILE.out 2>/dev/null`
	WA_REVOKED=` awk	'/^waRevoked/{print $2}'	$TMPFILE.out 2>/dev/null`
	WA_VALIDITY=`awk	'/^waValidity/{print $2}'	$TMPFILE.out 2>/dev/null`
	WA_LOGIN=`	 awk	'/^waLogin/{print $2}'		$TMPFILE.out 2>/dev/null`
	WA_PASSWORD=`awk	'/^waPassword/{print $2}'	$TMPFILE.out 2>/dev/null`
	WA_USEDBY=`	 awk	'/^waUsedBy/{print $2}'		$TMPFILE.out 2>/dev/null`
	WA_NOTAFTER=`awk	'/^waNotAfter/{print $2}'	$TMPFILE.out 2>/dev/null`

	YEAR=`	echo ${WA_NOTAFTER:0:4}`
	MONTH=`	echo ${WA_NOTAFTER:4:2}`
	DAY=`		echo ${WA_NOTAFTER:6:2}`
	HOUR=`	echo ${WA_NOTAFTER:8:2}`
	MINUTES=`echo ${WA_NOTAFTER:10:2}`

	# --------------------------------------------------------------------
	# GET POLICY FOR THIS CARD
	# --------------------------------------------------------------------
	WA_POLICY=`grep 'waPolicy: ' $TMPFILE.out | cut -f2 -d' ' 2>/dev/null`

	if [ -n "$WA_POLICY" ]; then
		POLICY=$WA_POLICY
	fi

	load_policy

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

	REQUEST_URI=print_user_data
	TITLE_TXT="Visualizzazione dati registrazione utente"

	print_page $1 "$REMAINING_TIME_MIN" "$REMAINING_TRAFFIC_MB" $WA_PASSWORD "$WA_NOTAFTER" $WA_VALIDITY $REVOKED $POLICY "$BACK_TAG"
}

status_network() {

	# --------------------------------------------------------------------------------------------
	# NB: bisogna metterlo in cron (5 minuti)...
	# --------------------------------------------------------------------------------------------
	# get_users_info
	# --------------------------------------------------------------------------------------------

	TMPFILE=/tmp/wi-auth-stat.$$

	# stefano 055340773 10.30.1.131:5280 00:e0:4c:d4:63:f5 10.30.1.105

	cat $DIR_CTX/*.ctx 2>/dev/null | sort > $TMPFILE 2>/dev/null

	# NB: wc se non legge da stdin stampa anche il nome del file...

	NUM_USERS=`			wc -l < $TMPFILE				   2>/dev/null`
	NUM_ACCESS_POINT=`wc -l < $ACCESS_POINT_LIST.up 2>/dev/null`

	if [ $NUM_USERS -gt 0 ]; then

		BODY=`cat $DIR_TEMPLATE/status_network_body.tmpl 2>/dev/null`

		while read AP UUID GATEWAY MAC IP
		do

			RIGA=`printf "$BODY" $UUID $IP $MAC $AP $AP 2>/dev/null`

			OUTPUT=`echo "$OUTPUT"; echo "$RIGA" 2>/dev/null`

		done < $TMPFILE
	fi

	TMP1=`cat $DIR_TEMPLATE/status_network_head.tmpl 2>/dev/null`
	TMP2=`date`
	TMP3=`printf "$TMP1" "$TMP2" $NUM_ACCESS_POINT $NUM_USERS 2>/dev/null`

	rm -f $TMPFILE 2>/dev/null

	TITLE_TXT="Firenze WiFi: stato rete"
	BODY_SHTML=` echo "$TMP3"; echo "$OUTPUT"; cat $DIR_TEMPLATE/status_network_end.tmpl 2>/dev/null`
}

update_ap_list() {

	# $1 -> ap
	# $2 -> public address to contact the access point

	ACCESS_POINT=`grep "^$1 " $ACCESS_POINT_LIST.up 2>/dev/null`

	if [ -z "$ACCESS_POINT" ]; then

		# si aggiunge access point alla lista di quelli contattabili...

		append_to_FILE "$1 $2" $ACCESS_POINT_LIST.up
	fi

	ACCESS_POINT=`grep "^$1 " $ACCESS_POINT_LIST.down 2>/dev/null`

	if [ -n "$ACCESS_POINT" ]; then

		LIST=`grep -v "^$1 " $ACCESS_POINT_LIST.down 2>/dev/null`

		if [ -n "$LIST" ]; then

			# si toglie access point dalla lista di quelli non contattabili...

			write_FILE "$LIST" $ACCESS_POINT_LIST.down
		else
			rm -f					 $ACCESS_POINT_LIST.down
		fi
	fi
}

start_ap() {

	# $1 -> ap
	# $2 -> public address to contact the access point

	# ------------------------------------------------------------------------------------------------------------------------------------------------
	# *.req (AP GATEWAY MAC IP REDIRECT TIMEOUT TOKEN UUID)
	# ------------------------------------------------------------------------------------------------------------------------------------------------
	# stefano 10.30.1.131:5280 00:e0:4c:d4:63:f5 10.30.1.105 http://www.google.com 0 10.30.1.105&1257603166&2a2436611f452f8eebddce4992e88f8d 055340773
	# ------------------------------------------------------------------------------------------------------------------------------------------------
	# *.cxt (AP UUID GATEWAY MAC IP)
	# ------------------------------------------------------------------------------------------------------------------------------------------------
	# stefano 055340773 10.30.1.131:5280 00:e0:4c:d4:63:f5 10.30.1.105
	# ------------------------------------------------------------------------------------------------------------------------------------------------

	LIST=`grep -l "^$1 " $DIR_REQ/*.req $DIR_CTX/*.ctx 2>/dev/null`

	if [ -n "$LIST" ]; then

		OP=QUIT
		LIST_SAFE=""

		for FILE in $LIST
		do
			unset GATEWAY

			SUFFIX="${FILE##*.}"

			if [ "$SUFFIX" = "req" ]; then

				read AP GATEWAY MAC IP REDIRECT TIMEOUT TOKEN UUID < $FILE 2>/dev/null

			elif [ "$SUFFIX" = "ctx" ]; then

				read AP UUID GATEWAY MAC IP < $FILE 2>/dev/null
			fi

			ACCESS_POINT_NAME=${AP##*@}

			if [ "$ACCESS_POINT_NAME" = "$1" ]; then

				LIST_SAFE="$FILE $LIST_SAFE"

				if [ "$SUFFIX" = "ctx" ]; then
					write_to_LOG "$UUID" "$AP" "$IP" "$MAC" "$TIMEOUT" "$TRAFFIC"
				fi
			fi
		done

		rm -f $LIST_SAFE
	fi

	update_ap_list "$1" "$2"

	BODY_SHTML="OK"
}

get_users_info() {

	# ------------------------------------------------------------------------------------------------------------------------------------------------
	# stefano 10.30.1.131:5280 00:e0:4c:d4:63:f5 10.30.1.105 http://www.google.com 0 10.30.1.105&1257603166&2a2436611f452f8eebddce4992e88f8d 055340773
	# ------------------------------------------------------------------------------------------------------------------------------------------------
	# cat $DIR_REQ/*.req 2>/dev/null | cut -f 1-2 -d' ' | uniq >/tmp/ACCESS_POINT.lst 2>/dev/null
	# ------------------------------------------------------------------------------------------------------------------------------------------------

	# NB: wc se non legge da stdin stampa anche il nome del file...

	NUM_ACCESS_POINT=`wc -l < $ACCESS_POINT_LIST.up 2>/dev/null`

	if [ $NUM_ACCESS_POINT -gt 0 ]; then

		TMPFILE=/tmp/nodog_check.$$

		while read AP GATEWAY
		do
			# we request nodog to check for users logout or disconnect...

			ask_nodog_to_check_for_users_info

		done < $ACCESS_POINT_LIST.up
	fi

	HTTP_RESPONSE_BODY="<html><body>OK</body></html>"
	HTTP_RESPONSE_HEADER="X-Sendfile: $TMPFILE\r\n"
}

status_ap() {

	# $1 -> ap

	AP=$1
	TMPFILE=/tmp/$1.html

	send_request_to_nodog "status" $TMPFILE

	if [ -s "$TMPFILE" ]; then
		HTTP_RESPONSE_BODY="<html><body>OK</body></html>"
		HTTP_RESPONSE_HEADER="X-Sendfile: $TMPFILE\r\n"
	fi
}

uploader() {

	# $1 -> path file uploaded

	mv $1 $HISTORICAL_LOG_DIR

	BODY_SHTML="OK"
}

reset_policy() {

	for POLICY in `ls $DIR_POLICY/* 2>/dev/null`
	do
		load_policy

		if [ -n "$POLICY_RESET" ]; then
			rm	-rf $DIR_CNT/$POLICY/* 2>/dev/null
		fi
	done

	BODY_SHTML="OK"
}

polling_attivazione() {

	# $1 WA_CELL

	ask_to_LDAP ldapsearch "-LLL -b $WIAUTH_CARD_BASEDN $LDAP_CARD_PARAM" "waUsedBy=$1" waPassword

	WA_PASSWORD=`grep 'waPassword:' $TMPFILE.out | cut -d':' -f2 2>/dev/null`

	INFO=""
	LOGIN_FORM=""
	TITLE="VERIFICA ATTIVAZIONE: ATTENDERE..."
	HEAD_HTML="<meta http-equiv=\"refresh\" content=\"3\">"
	CREDENTIALS_TAG="<p class=\"bigger\">&nbsp;</p><p class=\"bigger\">&nbsp;</p>"

	if [ -n "$WA_PASSWORD" ]; then

		INFO="
<p>Ti suggeriamo di prendere nota: </p>
<ul>
	<li>delle credenziali che ti serviranno ogni volta che vorrai accedere al servizio</li>
	<li><!--#echo var=\"$LOGOUT_NOTE\"--></li>
	<br/>
	<!--#echo var=\"$LOGOUT_HTML\"-->
</ul>
<br/>
<p class=\"bigger\">Ora puoi accedere al servizio cliccando il bottone</p>
"
		LOGIN_FORM="
<form action=\"/login\" method=\"post\">
<input type=\"hidden\" name=\"mac\" value=\"\">
<input type=\"hidden\" name=\"token\" value=\"\">
<input type=\"hidden\" name=\"redirect\" value=\"\">
<input type=\"hidden\" name=\"timeout\" value=\"\">
<input type=\"hidden\" name=\"gateway\" value=\"\">
<input type=\"hidden\" name=\"ap\" value=\"\">
<input type=\"hidden\" name=\"ip\" value=\"\">
<input type=\"hidden\" name=\"realm\" value=\"10_piazze\" />
<input type=\"hidden\" name=\"uid\" value=\"$1\">
<input type=\"hidden\" name=\"pass\" value=\"$WA_PASSWORD\">
<input type=\"image\" src=\"images/accedi.png\" name=\"submit\" value=\"Entra\" />
</form>
"

		HEAD_HTML=""
		TITLE="LE TUE CREDENZIALI SONO:"
		CREDENTIALS_TAG="<p class=\"bigger\">Utente: $1</p><p class=\"bigger\">Password: $WA_PASSWORD</p>"
	fi

	TITLE_TXT="Verifica attivazione"

	print_page "$TITLE" "$CREDENTIALS_TAG" "$INFO" "$LOGIN_FORM"
}

redirect_if_not_https() {

	return

 	if [ "$HTTPS" != "on" ]; then
 		HTTP_RESPONSE_BODY="<html><body>OK</body></html>"
 		HTTP_RESPONSE_HEADER="Refresh: 0; url=https://${HTTP_HOST}${REQUEST_URI}\r\n"

 		write_SSI
 	fi
}

uscita() {

	if [ -e $TMPFILE.out ]; then
		rm -f $TMPFILE.out
	fi
	if [ -e $TMPFILE.err ]; then
		rm -f $TMPFILE.err
	fi

	exit $EXIT_VALUE
}

write_SSI() {

	if [ -n "$CONNECTION_CLOSE" ]; then
		HTTP_RESPONSE_HEADER="Connection: close\r\n$HTTP_RESPONSE_HEADER"
	fi

	if [ -n "$SET_COOKIE" ]; then

		# ----------------------------------------------------------------------------------------------------------------------------
		# REQ: Set-Cookie: TODO[ data expire path domain secure HttpOnly ]
		# ----------------------------------------------------------------------------------------------------------------------------
		# string -- key_id or data to put into cookie   -- must
		# int    -- lifetime of the cookie in HOURS     -- must (0 -> valid until browser exit)
		# string -- path where the cookie can be used   --  opt
		# string -- domain which can read the cookie    --  opt
		# bool   -- secure mode                         --  opt
		# bool   -- only allow HTTP usage               --  opt
		# ----------------------------------------------------------------------------------------------------------------------------
		# RET: Set-Cookie: ulib.s<counter>=data&expire&HMAC-MD5(data&expire); expires=expire(GMT); path=path; domain=domain; secure; HttpOnly
		# ----------------------------------------------------------------------------------------------------------------------------

		HTTP_RESPONSE_HEADER="Set-Cookie: TODO[ $SET_COOKIE 4320 ]\r\n$HTTP_RESPONSE_HEADER" # 180 days
	fi

	if [ -n  "$HTTP_RESPONSE_HEADER" ]; then
		echo \"HTTP_RESPONSE_HEADER=$HTTP_RESPONSE_HEADER\"
	fi

	if [ -n "$HTTP_RESPONSE_BODY" ]; then
		echo  \"HTTP_RESPONSE_BODY=$HTTP_RESPONSE_BODY\"
	else
		if [ ! -d $DIR_SSI ]; then
			mkdir -p $DIR_SSI
		fi

		if [ -z "$BODY_SHTML" ]; then

			TITLE_TXT="400 Bad Request"
			HEAD_HTML=""
			BODY_SHTML="<h1>Bad Request</h1><p>Your browser sent a request that this server could not understand<br></p>"

			EXIT_VALUE=1
		fi

		echo -n -e "$HEAD_HTML"	 > $FILE_HEAD_HTML
		echo -n -e "$BODY_SHTML" > $FILE_BODY_SHTML

		if [ -z "$TITLE_TXT" ]; then
			TITLE_TXT="Firenze WiFi"
		fi

		echo -e "'TITLE_TXT=$TITLE_TXT'\nBODY_STYLE=$BODY_STYLE"
	fi

	uscita
}

do_cmd() {

	export TITLE_TXT HEAD_HTML BODY_SHTML BODY_STYLE \
			 SESSION_ID CONNECTION_CLOSE SET_COOKIE TMPFILE OUTPUT HTTP_RESPONSE_HEADER HTTP_RESPONSE_BODY \
			 OP FILE_CTX MAC IP GATEWAY AP TMP_FORM_FILE UUID CALLER_ID USER SIGNED_DATA UUID_TO_APPEND POLICY WA_UID CONSUME_ON

	# load environment

	CONF=$HOME/etc/$VIRTUAL_HOST/script.conf

   if [ -n "$CONF" -a -r "$CONF" ]; then
		. $CONF 2>/dev/null
	fi

	# --------------------------------
	#  session ID (no NAT)
	# --------------------------------
	SESSION_ID=$REMOTE_ADDR
	# --------------------------------
	#  session ID (with NAT)
	# --------------------------------
	#  if [ -n "$HTTP_COOKIE" ]; then
	#     SESSION_ID=$HTTP_COOKIE
	#  else
	#     SET_COOKIE=SESS_$$
	#     SESSION_ID=$SET_COOKIE
	#  fi
	# --------------------------------

	# check if we are operative...
	if [ -x $HOME/$VIRTUAL_HOST/ANOMALIA ]; then

		unset BACK_TAG

		message_page "$SERVICE" "$SERVICE per anomalia interna. Contattare l'assistenza: $TELEFONO"
	fi

	# -----------------------------------------------------------------------------------------------------------------------------------------------
	# GET /login?mac=00%3A14%3AA5%3A6E%3A9C%3ACB&ip=192.168.226.2&redirect=http%3A%2F%2Fgoogle&gateway=192.168.226.1%3A5280&timeout=0&token=x&ap=lab2
	# -----------------------------------------------------------------------------------------------------------------------------------------------
	# $1 -> mac
	# $2 -> ip
	# $3 -> redirect
	# $4 -> gateway
	# $5 -> timeout
	# $6 -> token
	# $7 -> ap
	# -----------------------------------------------------------------------------
	# 00:e0:4c:d4:63:f5 10.30.1.105 http://google 10.30.1.131:5280 stefano 0 x lab2
	# -----------------------------------------------------------------------------

	if [ "$REQUEST_METHOD" = "GET" ]; then

		case "$REQUEST_URI" in
			/start_ap)													start_ap					"$@"	;;
			/postlogin)													postlogin				"$@"	;;
			/stato_utente)												stato_utente					;;
			/registrazione)											registrazione_form			;;
			/polling_attivazione)									polling_attivazione	"$@"	;;
			/login | /cpe | /main | /personale | /circuito) main_page				"$@"	;;

			/logout)
				if [ $# -ge 8 ]; then 
					logout_notified_from_nodog "$@"
				else
					logout_request
				fi
			;;

			/card_activation)
				if [ "$REMOTE_ADDR" = "$PORTAL_IP_ADDRESS" ]; then
					card_activation "$@"
				fi
			;;
			/get_users_info)
				if [ "$REMOTE_ADDR" = "$PORTAL_IP_ADDRESS" ]; then
					get_users_info
				fi
			;;
			/reset_policy)
				if [ "$REMOTE_ADDR" = "$PORTAL_IP_ADDRESS" ]; then
					reset_policy
				fi
			;;

			/admin)
				redirect_if_not_https
				print_page
			;;
			/admin_status_network)
				redirect_if_not_https
				status_network
			;;
			/admin_status_ap)
				redirect_if_not_https
				status_ap "$@"
			;;
			/admin_printlog)
				redirect_if_not_https
				printlog
			;;
			/admin_view_user)
				REQUEST_URI=view_user
				TITLE_TXT="Visualizzazione dati utente"
				HEAD_HTML="<script type=\"text/javascript\" src=\"js/livevalidation_standalone.js\"></script>"

				print_page "$BACK_TAG"
			;;
			/admin_recovery)
				REQUEST_URI=recovery
				TITLE_TXT="Recovery utente"
				HEAD_HTML="<script type=\"text/javascript\" src=\"js/livevalidation_standalone.js\"></script>"

				print_page "$BACK_TAG"
			;;
			/admin_statistics)
				REQUEST_URI=statistics
				TITLE_TXT="Statistiche"

				print_page "$BACK_TAG"
			;;
			/admin_view_statistics_login)
				redirect_if_not_https
				view_statistics_login "$@"
			;;
			/admin_view_statistics_registration)
				redirect_if_not_https
				view_statistics_registration
			;;
			/admin_historical_statistics_login)
				redirect_if_not_https

				REQUEST_URI=historical_statistics_login
				historical_statistics_login	
			;;
			/admin_export_statistics_login_as_csv)
				redirect_if_not_https
				export_statistics_login_as_csv "$@"
			;;
			/admin_export_statistics_registration_as_csv)
				redirect_if_not_https
				export_statistics_registration_as_csv
			;;

			*) print_page ;;
		esac

	elif [ "$REQUEST_METHOD" = "POST" ]; then

		case "$REQUEST_URI" in
			/login)				login_request					"$@" ;;
			/logout)				logout_notified_from_popup "$@" ;;
			/uploader)			uploader							"$@" ;;
			/registrazione)	registrazione_request		"$@" ;;

			/admin_view_user)
				redirect_if_not_https
				view_user "$@"
			;;
			/admin_recovery)
				redirect_if_not_https

				REQUEST_URI=confirm_page
				TITLE_TXT="Conferma recovery"

				print_page "$TITLE_TXT" "$1" "admin_execute_recovery" "$1" "$BACK_TAG"
			;;
			/admin_execute_recovery)
				redirect_if_not_https
				execute_recovery "$@"
			;;
		esac
	fi

	write_SSI
}
#-----------------------------------
# END FUNCTION
#-----------------------------------

DEBUG=on

if [ -z "$DEBUG" ]; then
   do_cmd "$@"
else
	TMPFILE=`basename $0`
	TMPFILE_OUT=/tmp/$TMPFILE_$$.out
	TMPFILE_ERR=/tmp/$TMPFILE_$$.err
	(
	echo "ENVIRONMENT:"
	echo "-----------------------------------------------------------"
	env
	echo "-----------------------------------------------------------"
	echo "STDERR:"
	echo "-----------------------------------------------------------"
	) 2>$TMPFILE_ERR >&2
	(
   set -x
   do_cmd "$@"
   set +x
   ) > $TMPFILE_OUT 2>>$TMPFILE_ERR
   echo "-----------------------------------------------------------" 2>>$TMPFILE_ERR >&2
	cat $TMPFILE_OUT
fi
