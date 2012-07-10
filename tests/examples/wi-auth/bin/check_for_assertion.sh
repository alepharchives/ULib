#/bin/bash

# check_for_assertion.sh

sync
echo ---------------------------------
grep assert /tmp/userver-firenze_*.err
echo ---------------------------------
grep -i 'ERROR\|ABORT\|ASSERT' /var/log/userver-firenze_*.log /var/log/uclient-firenze.log | grep -v 'SSL EOF observed that violates the protocol' | grep -v 'remove of SSL session on db failed with error -2'
echo ---------------------------------
grep 'Bad Req' /var/log/userver-firenze_*.log
echo ---------------------------------
# zcat /var/log/userver-firenze_*.gz | grep -i 'ERROR\|ABORT\|ASSERT' | grep -v 'SSL EOF observed that violates the protocol' | grep -v 'remove of SSL session on db failed with error -2'
echo ---------------------------------
# zcat /var/log/wi-auth-logs-archives/*.gz | grep -i 'ERROR\|ABORT\|ASSERT'
echo ---------------------------------
