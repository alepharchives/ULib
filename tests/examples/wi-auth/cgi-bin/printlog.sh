#!/bin/bash

# printlog.sh

. ./.env

if [ "$REQUEST_METHOD" = "GET" ]; then

	echo -e "Content-Type: text/plain; charset=us-ascii\r\n\r\n"

	if [ -s $FILE_LOG ]; then

		# 2009/11/07 13:14:35 op: PASS_AUTH, uid: 3397363258, ap: dev, ip: 10.30.1.105, mac: 00:e0:4c:d4:63:f5, timeout: 86400, traffic: 300

		awk '
		{
		for (f = 1 ; f <= NF ; f++)
			{
			row[NR,f] = $f

			l = length($f) ; if (l > max[f]) max[f] = l
			}

		if (NF > maxNF) maxNF = NF
		}

		END {
			for (r = (NR > 200 ? NR : 201) - 200 ; r <= NR ; r++)
				for (f = 1 ; f <= maxNF ; f++)
					printf "%-*s%s",
									 (max[f] > 999 ? 999 : max[f]),
						(length(row[r,f]) > 999 ? substr(row[r,f], 1, 999 -3) "..." : row[r,f]),
						(f < maxNF ? " " : "\n")
		}
		' $FILE_LOG

	fi

	uscita

fi

write_OUTPUT
