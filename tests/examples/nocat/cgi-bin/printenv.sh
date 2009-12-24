#!/bin/sh

# printenv -- demo CGI program which just prints its environment

echo -e 'Content-Type: text/html; charset=utf8\r\n\r'
echo '<pre>'
env
echo '</pre>'
