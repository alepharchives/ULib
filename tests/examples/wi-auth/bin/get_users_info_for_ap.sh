#/bin/bash

# get_users_info_for_ap.sh

PROGRAM=`basename $0 .sh`

exec /home/unirel/userver/bin/send_req_to_portal.sh $PROGRAM "get_users_info?ap=$1\&gateway=$2"
