#/bin/bash

# get_users_info.sh

# environment
source /srv/wifi-portal-firenze/etc/environment.conf

PROGRAM=`basename $0 .sh`

exec $WIFI_PORTAL_HOME/bin/send_req_to_portal.sh $PROGRAM $PROGRAM
