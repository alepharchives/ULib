#!/bin/sh

NAME=$1

if [ ! -f $NAME.eml ]; then
	exit 1
fi

 TO=`head   $NAME.eml | grep To   | cut -d' ' -f2 | tr -d "[:space:]"`
 FROM=`head $NAME.eml | grep From | cut -d' ' -f2 | tr -d "[:space:]"`

if [ ! -f $NAME.p7m ]; then
	openssl smime -in $NAME.eml -sign -signer $RA_OPERATOR_CERTIFICATE -inkey $RA_OPERATOR_KEY -nodetach -binary -outform SMIME >$NAME.p7m 2>/dev/null

	# if [ $? -eq 0 ]; then
		# rm -f $NAME.eml
	# fi

	# command for extract mail
	# openssl smime -verify -noverify -in $NAME.p7m > $NAME.eml
fi

# --------------------------------------------
# smtp configuration
# --------------------------------------------

# TEST
# openssl s_client -connect ${SMTP_HOST}:${SMTP_PORT} -starttls smtp
# exit 0
# --------------------------------------------

msmtp --tls=on --tls-certcheck=off --tls-min-dh-prime-bits=512 --host="${SMTP_HOST}" --port="${SMTP_PORT}" --from="${FROM}" -- "${TO}" <$NAME.p7m >/dev/null 2>&1

if [ $? -eq 0 ]; then
	cp $NAME.p7m /tmp/$NAME.p7m.$$
	# rm -f $NAME.p7m
	exit 0
#else
#	sleep 60
fi

exit 1
