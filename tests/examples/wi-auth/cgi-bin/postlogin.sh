#!/bin/bash

# postlogin.sh

. ./.env

# set -x

if [ "$REQUEST_METHOD" = "GET" ]; then

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
			message_page "PostLogin" "Sei gia' loggato! (postlogin)"
		fi

		# stefano 10.30.1.131:5280 00:e0:4c:d4:63:f5 10.30.1.105 http://www.google.com 0 10.30.1.105&1257603166&2a2436611f452f8eebddce4992e88f8d 055340773

		read AP GATEWAY MAC IP REDIRECT TIMEOUT TOKEN UUID < $REQ_FILE

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

		print_page "$1" "$2" "$3" "$1" "$3" "$3"

	elif [ $# -eq 2 ]; then

		# $1 -> uid
		# $2 -> gateway

		CONNECTION_CLOSE=1

		BASE_NAME=logout_popup

		print_page "$1"

	fi

fi

write_OUTPUT
