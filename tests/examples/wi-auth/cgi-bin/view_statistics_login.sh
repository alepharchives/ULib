#!/bin/bash

# view_statistics_login.sh

. ./.env

if [ "$REQUEST_METHOD" = "GET" ]; then

	BASE_NAME=view_statistics
	HREF_TAG="/export_statistics_login_as_csv"

	if [ -n "$1" ]; then
		HREF_TAG="$HREF_TAG?file=$1"
		FILE_LOG=$HISTORICAL_LOG_DIR/$1
		COMMAND=$UNCOMPRESS_COMMAND_HISTORICAL_LOGS
		TITLE="Numero LOGIN per Access Point: file $1"
	else
		COMMAND=cat
		TITLE="Numero LOGIN per Access Point"
	fi

#	if [ -s $FILE_LOG ]; then
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
#	fi

	print_page "$TITLE" "$TITLE" "$TABLE" "$BACK_TAG" "<a class=\"back\" href=\"$HREF_TAG\">Esporta in formato CSV</a>"

fi

write_OUTPUT
