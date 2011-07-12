#!/bin/bash

# set -x
# env >/tmp/main.bash.env

# load common function
. ./.functions

# ----------------------------------
# GET /ponza_login.kl1?mac=00%3A14%3AA5%3A6E%3A9C%3ACB&ip=192.168.226.2&redirect=http%3A%2F%2Fwww.google.com%2Fwebhp&gateway=192.168.226.1%3A5280&timeout=86400&token=lOosGl9h1aHxo&ap=lab2.wpp54
# ----------------------------------
# $1 -> mac
# $2 -> ip
# $3 -> redirect
# $4 -> gateway
# $5 -> timeout
# $6 -> token
# $7 -> ap
# ----------------------------------
# 00:e0:4c:d4:63:f5 10.30.1.105 http://www.google.com 10.30.1.131:5280 stefano 86400 lOosGl9h1aHxo lab2.wpp54
# ----------------------------------

if [ "$REQUEST_METHOD" = "GET" -a $# -eq 7 ]; then

	user_has_valid_MAC "$@"

	if [ -n "$SSL_CLIENT_CERT_SERIAL" ]; then
		user_has_valid_cert "$@"
	fi

	main_page "$@"

fi

write_SSI
