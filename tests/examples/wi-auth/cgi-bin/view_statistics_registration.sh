#!/bin/bash

# view_statistics_registration.sh

. ./.env

if [ "$REQUEST_METHOD" = "GET" ]; then

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

	BASE_NAME=view_statistics

	TITLE="Numero Registrazioni per data"
	HREF_TAG="/export_statistics_registration_as_csv"

	print_page "$TITLE" "$TITLE" "$TABLE" "$BACK_TAG" "<a class=\"back\" href=\"$HREF_TAG\">Esporta in formato CSV</a>"

fi

write_OUTPUT
