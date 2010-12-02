#!/bin/bash

# polling_attivazione.sh

. ./.env

if [ $# -ge 1						 -a \
	  "$REQUEST_METHOD" = "GET" -a \
	  "$REQUEST_URI"	  = "/$BASE_NAME" ]; then

#	set -x

	# $1 WA_CELL

	ask_to_LDAP ldapsearch "-LLL -b $WIAUTH_CARD_BASEDN $LDAP_CARD_PARAM" "waUsedBy=$1" waPassword

	WA_PASSWORD=`grep 'waPassword:' $TMPFILE.out | cut -d':' -f2 2>/dev/null`

	if [ -n "$WA_PASSWORD" ]; then

		CREDENTIALS_TAG="<p class=\"bigger\">Utente: $1</p><p class=\"bigger\">Password: $WA_PASSWORD</p>"
		TITLE="LE TUE CREDENZIALI SONO:"
		REFRESH_TAG=""

		INFO="
<p>Ti suggeriamo di prendere nota: </p>
<ul>
<li>
delle credenziali che ti serviranno ogni volta che vorrai accedere al servizio
</li>
<li>
della pagina di uscita (logout), magari inserendola tra i tuoi segnalibri (bookmarks). Se utilizzerai questa pagina, quando hai finito di navigare, il tempo e traffico della tua navigazione, e quindi quanto ti rimane, saranno corrispondenti a quelli effettivi
</li>
<p class=\"bigger\">La pagina di uscita (logout): http://$HTTP_HOST/logout_page</p>
</ul>
<br/>
<p class=\"bigger\">Ora puoi accedere al servizio cliccando il bottone</p>
"

		LOGIN_FORM="
<form action=\"/login\" method=\"post\">
<input type=\"hidden\" name=\"realm\" value=\"10_piazze\" />
<input type=\"hidden\" name=\"uid\"   value=\"$1\">
<input type=\"hidden\" name=\"pass\"  value=\"$WA_PASSWORD\">
<input type=\"image\" src=\"wi-auth/images/accedi.gif\" name=\"submit\" value=\"Entra\" />
</form>
"

		print_page  "$REFRESH_TAG" "$TITLE" "$CREDENTIALS_TAG" "$INFO" "$LOGIN_FORM"

	else

		CREDENTIALS_TAG="<p class=\"bigger\">&nbsp;</p><p class=\"bigger\">&nbsp;</p>"
		TITLE="VERIFICA ATTIVAZIONE: ATTENDERE..."
		REFRESH_TAG="<meta http-equiv=\"refresh\" content=\"3\">"

		print_page  "$REFRESH_TAG" "$TITLE" "$CREDENTIALS_TAG" "" ""

	fi

fi

write_OUTPUT
