#!/bin/bash

# historical_statistics_login.sh

. ./.env

# set -x

if [ $# -eq 0 -a \
     "$REQUEST_METHOD" = "GET" ]; then

	TABLE_TAG_START="<table class=\"centered\" border=\"1\">"
	TABLE_TAG_END="</table>"
	TR_TAG_START="<tr>"
	TR_TAG_END="</tr>"
	TD_HEADER_TAG_START="<td class=\"header_smaller\">"
	TD_DATA_TAG_START="<td class=\"data_smaller\" align=\"right\">"
	TD_TAG_END="</td>"
	TH_TAG_START="<th class=\"header_smaller\">"
	TH_TAG_END="</th>"
	URL_TAG_START="<a href=\"/view_statistics_login?file=%s\">%s</a>"

	TABLE="$TABLE_TAG_START\n\t$TH_TAG_START\n\t\tARCHIVI\t$TH_TAG_END\n"

	for file in `ls -rt $HISTORICAL_LOG_DIR/$REGEX_HISTORICAL_LOGS`
	do
		filename=`basename $file`
		TAG=`printf "$URL_TAG_START" $filename $filename`

		TABLE="$TABLE\t$TR_TAG_START\n\t\t$TD_DATA_TAG_START\n\t\t\t$TAG\n\t\t$TD_TAG_END\n\t$TR_TAG_END\n"
	done

	TABLE=`echo -e "$TABLE$TABLE_TAG_END"`

	print_page "Storico" "Storico" "$TABLE" "$BACK_TAG"

fi

write_OUTPUT
