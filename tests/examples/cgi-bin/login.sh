#!/bin/sh

# login.sh

if [ "$REQUEST_METHOD" = "POST" -a $# -eq 2 ]; then

	# REQ: Set-Cookie: TODO[ data expire path domain secure HttpOnly ]
	# ----------------------------------------------------------------------------------------------------------------------------
	# string -- Data to put into the cookie         -- must
	# int    -- Lifetime of the cookie in HOURS     -- must (0 -> valid until browser exit)
	# string -- Path where the cookie can be used   --  opt
	# string -- Domain which can read the cookie    --  opt
	# bool   -- Secure mode                         --  opt
	# bool   -- Only allow HTTP usage               --  opt
	# ----------------------------------------------------------------------------------------------------------------------------
	# RET: Set-Cookie: ulib_sid=data&expire&HMAC-MD5(data&expire); expires=expire(GMT); path=path; domain=domain; secure; HttpOnly
 
	echo -e "Connection: close\r"
	echo -e "Set-Cookie: TODO[ \"$1\" 24 ]\r\n\r"

	exit 0
fi

exit 1
