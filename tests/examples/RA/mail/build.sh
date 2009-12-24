#!/bin/sh

# create PDF...

tiffcp -a *.tiff out.tiff >/dev/null 2>&1

PDF=`tiff2pdf -p A4 -q 100 -j out.tiff | openssl base64 -e 2>/dev/null` # Creating PDF file and convert in base64...

printf "`cat mail-cpe.tmpl`" "$v1" "$v2" "$v3" "$v4" "$v5" "$v6" "$v7" "$v8" "$v9" "${v10}" "${v11}" "${v12}" \
									  "${v13}" "${v14}" "${v15}" "${v16}" "${v17}" "${v18}" "${v19}" "${v20}" "${v21}" "${v22}" "${v24}" \
									  "$PDF" > send_to_$$.eml
