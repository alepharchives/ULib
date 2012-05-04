#/bin/bash

# check_for_assertion.sh

echo ---------------------------------
grep assert /tmp/userver-firenze_*.err
echo ---------------------------------
grep -i 'ERROR\|ABORT\|ASSERT' /var/log/userver-firenze_* /var/log/uclient-firenze.log | grep -v 'SSL EOF observed that violates the protocol'
echo ---------------------------------
