#!/bin/sh

# $1 is the type of the event wich can be SENT, RECEIVED, FAILED or REPORT.
# $2 is the filename of the sms.
# $3 is the message id. Only used for SENT messages with status report.

# echo "SMS event: $1 (filename: $2)"

if [ "$1" = "RECEIVED" -a -s $2 ]; then

	SMS=`cat $2`

	PHONE_NUMBER=`echo "$SMS" | grep 'From:' | cut -f2 -d' '`

	if [ -n "$PHONE_NUMBER" ]; then

		TIMEOUT=2

		printf "CARD00000001ARGV%08x%s" ${#SMS} "$SMS" >/tmp/LAST_REQUEST

#					 nc -w $TIMEOUT -l	  -p 4433		  </tmp/RESPONSE >/dev/null &
#					 PID=$!
#					 sleep 1

		RESPONSE=`nc -w $TIMEOUT localhost 4433		  </tmp/LAST_REQUEST`

		if [ -n "$RESPONSE" ]; then

			FILE=`mktemp /var/spool/sms/outgoing/send_XXXXXX`

			echo "To: $PHONE_NUMBER"					 >$FILE
			echo												>>$FILE
			echo `expr substr "$RESPONSE" 13 5000`	>>$FILE

#			kill $PID
#			wait $PID

			exit 0

		fi

	fi

fi

exit 1
