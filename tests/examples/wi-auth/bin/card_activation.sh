#/bin/bash

# card_activation.sh

# environment
source /srv/wifi-portal-firenze/etc/environment.conf

PROGRAM=`basename $0 .sh`

CALLER_ID=`echo -n $1 | perl -e "use URI::Escape; print uri_escape(join('',<>));"`

exec $WIFI_PORTAL_HOME/bin/send_req_to_portal.sh $PROGRAM "card_activation?caller_id=$CALLER_ID"
