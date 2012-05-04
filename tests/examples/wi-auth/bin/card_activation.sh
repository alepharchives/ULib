#/bin/bash

# card_activation.sh
set -x

(
PROGRAM=`basename $0 .sh`

CALLER_ID=`echo -n $1 | perl -e "use URI::Escape; print uri_escape(join('',<>));"`

exec /home/unirel/userver/bin/send_req_to_portal.sh $PROGRAM "card_activation?caller_id=$CALLER_ID"
)>& /tmp/attivation.err
