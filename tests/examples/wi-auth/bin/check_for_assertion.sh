#/bin/bash

# check_for_assertion.sh

sync
echo -----------------------------------------------------------------------------------------------------------------------------------
grep --colour -i -E -e assert /tmp/userver-firenze_*.err
echo -----------------------------------------------------------------------------------------------------------------------------------
grep --colour -i -E -e 'ERROR|ABORT|ASSERT' /var/log/userver-firenze_*.log /var/log/uclient-firenze.log | grep -v 'SSL EOF observed that violates the protocol'
echo -----------------------------------------------------------------------------------------------------------------------------------
grep --colour -i -E -e 'Bad Req| write ' /var/log/userver-firenze_*.log
echo -----------------------------------------------------------------------------------------------------------------------------------
# zcat /var/log/userver-firenze_*.gz | grep --colour -i -E -e 'ERROR|ABORT|ASSERT' | grep -v 'SSL EOF observed that violates the protocol'
echo -----------------------------------------------------------------------------------------------------------------------------------
# zcat /var/log/wi-auth-logs-archives/*.gz | grep --colour -i -E -e 'ERROR|ABORT|ASSERT'
echo -----------------------------------------------------------------------------------------------------------------------------------
