#!/bin/sh

NAME=$1/send_to_$2
DIR_CERT=cert

if [ ! -f $NAME.eml ]; then
	exit 1
fi

 TO=`head   $NAME.eml | grep To   | cut -d' ' -f2 | tr -d "[:space:]"`
#FROM=`head $NAME.eml | grep From | cut -d' ' -f2 | tr -d "[:space:]"`
 FROM=ra-station@unirel.com

if [ ! -f $NAME.p7m ]; then
	openssl smime -in $NAME.eml -sign -signer $DIR_CERT/cert.pem -inkey $DIR_CERT/key.pem -nodetach -binary -outform SMIME >$NAME.p7m 2>/dev/null

	if [ $? -eq 0 ]; then
		rm -f $NAME.eml
	fi

	# command for extract mail
	# openssl smime -verify -noverify -in $NAME.p7m > $NAME.eml
fi

# --------------------------------------------
# smtp configuration
# --------------------------------------------
PORT=25
 SMTP=192.168.220.11
#SMTP=mail.t-unwired.com

# TEST
# openssl s_client -connect ${SMTP}:${PORT} -starttls smtp
# exit 0
# --------------------------------------------

msmtp --tls=on --tls-certcheck=off --tls-min-dh-prime-bits=512 --host="${SMTP}" --port="${PORT}" --from="${FROM}" -- "${TO}" <$NAME.p7m >/dev/null 2>&1

if [ $? -eq 0 ]; then
	rm -f $NAME.p7m
	exit 0
#else
#	sleep 60
fi

exit 1
