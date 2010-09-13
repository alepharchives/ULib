#!/bin/sh

echo pippo pluto paperino > /tmp/sendfile.txt

echo -e "X-Powered-By: PHP/5.2.6-pl7-gentoo\r\nX-Sendfile: /tmp/sendfile.txt\r\nX-Powered-By: PHP/5.2.6-pl7-gentoo\r\n\r"
