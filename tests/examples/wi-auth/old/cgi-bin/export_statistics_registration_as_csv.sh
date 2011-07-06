#!/bin/bash

# export_statistics_registration_as_csv.sh

. ./.env

if [ "$REQUEST_METHOD" = "GET" ]; then

	echo -e "Content-Type: text/csv; charset=us-ascii\r\n\r"

	ask_to_LDAP ldapsearch "-LLL -b $WIAUTH_CARD_BASEDN $LDAP_CARD_PARAM" "waNotAfter=*" waNotAfter

	awk '
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

		printf "\"%s\"", "Data";
		printf ",";
		printf "\"%s\"", "Registrazioni";
		printf "\n";

		for (i = 1; i <= n; i++) {

			printf "\"%s\"", sorted_registrations[i];
			printf ",";
			printf "%.0f", registrations[sorted_registrations[i]];
			printf "\n";
		}

		printf "\"%s\"", "Totale";
		printf ",";
		printf "%.0f", total;
		printf "\n";

	}
	' $TMPFILE.out

	uscita

fi

write_OUTPUT
