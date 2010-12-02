#!/bin/bash
#
# last modified: 2009/10/25
#

mailrecipients='stefano@unirel.com marcod@unirel.com leonardo@unirel.com'

_debug() { true ; }
_debug() { echo "$*" ; }

_mail() { true ; }
_mail() {
	echo "$*" |
		mail -r noreply@`uname -n` \
			-s "userver: $ctx $req message from $REMOTE_ADDR" \
			$mailrecipients
}

log() {
	test $# -lt 3 && {
		_debug 'ERROR: specify log context and log message' ; return ; }

	req=$1
	ctx=$2 ; test $ctx != nodog && {
		_debug 'ERROR: invalid log context' ; return ; }

	logfile=/var/log/userver.$ctx.log

	shift 2

	msg="`date '+%Y/%m/%d %H:%M:%S'`: $REMOTE_ADDR: log message: $*"
	_debug msg: "$msg"

	echo "$msg" >> $logfile

	_mail "$msg"
}


echo -e 'Content-Type: text/html; charset=utf8\r\n\r'

echo '<pre>'

_debug REMOTE_ADDR: $REMOTE_ADDR
_debug REQUEST_URI: $REQUEST_URI
_debug ARGS: "$@"

case $1 in
	log) log $@ ;;
	*) _debug 'ERROR: invalid req' ;;
esac

echo done.

echo '</pre>'

exit 0
