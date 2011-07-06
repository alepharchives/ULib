#!/bin/bash

# export_statistics_login_as_csv.sh

. ./.env

if [ "$REQUEST_METHOD" = "GET" ]; then

	COMMAND=cat

	if [ -n "$1" ]; then
		COMMAND=$UNCOMPRESS_COMMAND_HISTORICAL_LOGS
		FILE_LOG="$HISTORICAL_LOG_DIR/$1"
	fi

	echo -e "Content-Type: text/csv; charset=us-ascii\r\n\r"

#	if [ -s $FILE_LOG ]; then

		$COMMAND $FILE_LOG | awk '
		/LOGIN/ { a=$8; gsub(",","",a) ; login[a $1]+=1 ; if (!date[$1]) date[$1]+=1 ; if (!ap[a]) ap[a]+=1 }

		END {
			n=asorti(date, sorted_date);

			printf "\"\","; 
			
			for (i = 1; i <= n; i++) {
				printf "\"%s\",", sorted_date[i] 
			}; 
			
			printf "\n"
			
			for (j in ap) { 

				printf "\"AP %s\",", j
				
				for (i = 1; i <= n; i++) {
					printf "%.0f,", login[j sorted_date[i]] 
				}

				printf "\n"
			}
		}
		'
#	fi

	uscita

fi

write_OUTPUT
