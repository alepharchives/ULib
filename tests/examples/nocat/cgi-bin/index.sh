#!/bin/sh

# index.sh

# set -x

if [ $# -eq 7 -a \
	  "$REQUEST_METHOD" = "GET" ]; then

		# $1 -> mac
		# $2 -> ip
		# $3 -> redirect
		# $4 -> gateway
		# $5 -> timeout
		# $6 -> token
		# $7 -> ap

		if [ "$REQUEST_URI" = "/login" ]; then

		# --------------------------------------------------------------------------------
		# NB: we need this for the old version...
		# --------------------------------------------------------------------------------
		#	if [ "$3" = "http://192.168.112.254:5280/images/firenze-wifi2.png" -o
		#		  "$3" = "http://192.168.112.254:5280/images/partecipaa20venti.png" ]; then
		#
		#		echo -e "Content-Type: image/png; charset=binary\r\n\r"
		#
		#		if [ "$3" = "http://192.168.112.254:5280/images/firenze-wifi2.png" ]; then
		#			cat /usr/lib/nodog/images/firenze-wifi2.png
		#		else
		#			cat /usr/lib/nodog/images/partecipaa20venti.png
		#		fi
		#
		#		exit 0
		#	fi
		# --------------------------------------------------------------------------------

			OUTPUT=`cat ../etc/nodog_index.tmpl 2>/dev/null`

		#	OUTPUT=`printf "$OUTPUT" "http://$HTTP_HEADER_HOST/cgi-bin/index.sh?$QUERY_STRING" 2>/dev/null`
			OUTPUT=`printf "$OUTPUT" "http://$HTTP_HOST/cgi-bin/index.sh?$QUERY_STRING" 2>/dev/null`

			echo -e "Content-Type: text/html; charset=utf-8\r\n\r"
			echo -n -E "$OUTPUT"

			exit 0

	#	elif [ -z "$REQUEST_URI" ]; then
		elif [ "$REQUEST_URI" = "/cgi-bin/index.sh" ]; then

		#	/usr/lib/nodog/firewall/access.fw permit $1 $2 Member
			../firewall/nodog.fw					 permit $1 $2 Member

			echo -e "Refresh: 0; url=http://www.google.com\r\n\r\n<html><body>OK</body></html>"

			exit 0
		fi
fi

echo -e "Status: 400\r\n\r\n" \
        "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\r\n" \
        "<html><head>\r\n" \
        "<title>400 Bad Request</title>\r\n" \
        "</head><body>\r\n" \
        "<h1>Bad Request</h1>\r\n" \
        "<p>Your browser sent a request that this server could not understand.<br />\r\n" \
        "</p>\r\n" \
        "<hr>\r\n" \
        "<address>ULib Server</address>\r\n" \
        "</body></html>\r"

exit 1
