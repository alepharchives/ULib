ULib - C++ library
================================

What's this?
------------

ULib is a highly optimized class framework for writing C++ applications. I wrote this framework as my tool for writing applications in various contexts.
It is a result of many years of work as C++ programmer. I think, in my opinion, that its strongest points are simplicity, efficiency and sophisticate
debugging. This framework offers a class foundation that disables language features that consume memory or introduce runtime overhead, such as rtti and
exception handling, and assumes one will mostly be linking applications with other pure C based libraries rather than using the overhead of the standard
C++ library and other similar class frameworks. It include as application example a powerful search engine with relative web interface and a multi purpose
server (plugin oriented) which results, out of John Fremlin accurate investigations (http://john.freml.in/ulib-fast-io-framework), to be one of the faster
web application frameworks for serving small dynamic webpages (and also make easier the usage of shell scripts for CGI application)

userver_(tcp|ssl|ipc) multi purpose server (plugin oriented)
------------------------------------------------------------

The current version offers the following features :

    * HTTP/1.0 and 1.1 protocols supported.
    * Persistent connections for HTTP/1.1 and Keep-Alive support for HTTP/1.0.
    * Browser cache management (headers: If-Modified-Since/Last-modified).
    * Chunk-encoding transfers support.
    * HTTP multi-range request support.
    * Memory caching for small static pages with smart compression (and on Linux automatic update with inotify).
    * Accept HTTP uploads up to 4 GB without increasing memory usage.
    * Support for pipelining.
    * Support for virtual hosts (also with SSL).
    * Support for basic/digest authentication.
    * Support for directory listings via basic/digest authentication.
    * Support for uri protection.
    * Support for aliases/redirection.
    * Support for RewriteRule (lighttpd-like) that check for file existence as they do on Apache,
      some CMS (SilverStripe) require it.
    * Support for JSONRequest (http://json.org/JSONRequest.html).
    * Support for upload progress via USP (ULib Servlet Page).
    * CGI support for shell script processes (with automatic management of form and cookie).
    * General CGI support (run any CGI script) with automatic output compression (using deflate method).
    * CGI support for the X-Sendfile feature and also supports X-Accel-Redirect headers transparently.
    * Web Socket support (experimental).
    * Support for Windows (without preforking).
    * Requests cut in phases for modular architecture (apache-like).
    * Built-in modules :
          o mod_rpc : generic Remote Procedure Call.
          o mod_http : core features, static file handler and dynamic page (ULib Servlet Page).
          o mod_tsa : server side time stamp support.
          o mod_echo : echo features.
          o mod_soap : generic SOAP server services support.
          o mod_nocat : captive portal implementation.
          o mod_fcgi : third-party applications support thru FastCGI interface.
          o mod_scgi : module that implements the client side of the SCGI protocol (experimental).
          o mod_ssi : Server Side Includes support (experimental).
          o mod_shib : web single sign-on support (experimental).
          o mod_proxy : proxy support (experimental).
          o mod_geoip : geolocation support (experimental).
          o mod_stream : simple streaming support (experimental).
          o mod_socket : web sockets application framework (experimental).
    * Configuration file with dedicated section.
# ----------------------------------------------------------------------------------------------------------------------------------------
# userver - configuration parameters
# ----------------------------------------------------------------------------------------------------------------------------------------
# USE_IPV6      flag to indicate use of ipv6
# SERVER        host name or ip address for the listening socket
# PORT          port number             for the listening socket
# SOCKET_NAME   file name               for the listening socket
# IP_ADDRESS    public ip address of host for the interface connected to the Internet (autodetected if not specified)
# ALLOWED_IP    client address IP-based access control (IPADDR[/MASK])
#
# USE_TCP_OPTIMIZATION  flag indicating the use of TCP/IP options to optimize data transmission (NODELAY, DEFER_ACCEPT, QUICKACK)
#
# PID_FILE      write pid on file indicated
# WELCOME_MSG   message of welcome to send initially to client
# RUN_AS_USER   downgrade security to that user account
# DOCUMENT_ROOT The directory out of which you will serve your documents
#
# LOG_FILE      locations for file log
# LOG_FILE_SZ   memory size for file log
# LOG_MSG_SIZE  limit length of print network message to LOG_MSG_SIZE chars (default 128)
#
# PLUGIN        list of plugins to load, a flexible way to add specific functionality to the server
# PLUGIN_DIR    directory of plugins to load
#
# REQ_TIMEOUT   timeout for request from client
# CGI_TIMEOUT   timeout for cgi execution
#
# MAX_KEEP_ALIVE Specifies the maximum number of requests that can be served through a Keep-Alive (Persistent) session.
#                (Value <= 1 will disable Keep-Alive)
#
# CERT_FILE     certificate of server
# KEY_FILE      private key of server
# PASSWORD      password for private key of server
# CA_FILE       locations of trusted CA certificates used in the verification
# CA_PATH       locations of trusted CA certificates used in the verification
#
# VERIFY_MODE   mode of verification (SSL_VERIFY_NONE=0, SSL_VERIFY_PEER=1, SSL_VERIFY_FAIL_IF_NO_PEER_CERT=2, SSL_VERIFY_CLIENT_ONCE=4)
# ----------------------------------------------------------------------------------------------------------------------------------------
# how to verify peer certificates. The possible values of this setting are:
#
# SSL_VERIFY_NONE                 - do not verify anything
# SSL_VERIFY_PEER                 - verify the peer certificate, if one is presented
# SSL_VERIFY_FAIL_IF_NO_PEER_CERT - require a peer certificate, fail if one is not presented
#
# SSL/TLS servers will usually set VERIFY_MODE to SSL_VERIFY_NONE.
# SSL/TLS clients will usually set VERIFY_MODE to SSL_VERIFY_FAIL_IF_NO_PEER_CERT.
# ----------------------------------------------------------------------------------------------------------------------------------------
#
# PREFORK_CHILD number of child server processes created at startup ( 0 - serialize, no forking
#                                                                     1 - classic, forking after accept client)
#                                                                    >1 - pool of process serialize plus monitoring process)
# ----------------------------------------------------------------------------------------------------------------------------------------

userver {

# USE_IPV6       no
# SERVER         localhost
# PORT           80
# SOCKET_NAME    tmp/socket
# IP_ADDRESS     10.30.1.131
# ALLOWED_IP     127.0.0.1,10.30.0.0/16

  USE_TCP_OPTIMIZATION yes

  PID_FILE       /var/run/userver.pid
# WELCOME_MSG    "220 david.unirel.intranet ULib WEB server (Version 1.1.0) ready.\n"
# RUN_AS_USER    apache
  DOCUMENT_ROOT  /var/www/localhost/htdocs

  LOG_FILE       /var/log/userver.log
  LOG_FILE_SZ    1M
  LOG_MSG_SIZE   -1

  PLUGIN                     mod_http
# PLUGIN         "mod_tsa    mod_http"
# PLUGIN         "mod_rpc    mod_http"
# PLUGIN         "mod_soap   mod_http"
# PLUGIN         "mod_fcgi   mod_http"
# PLUGIN         "mod_scgi   mod_http"
# PLUGIN         "mod_proxy  mod_http"
# PLUGIN         "mod_geoip  mod_http"
# PLUGIN         "mod_stream mod_http"
# PLUGIN         "mod_socket mod_http"

# PLUGIN_DIR     /usr/local/libexec/ulib

  REQ_TIMEOUT    30
  CGI_TIMEOUT    60

  MAX_KEEP_ALIVE 1024

# CERT_FILE      ../ulib/CA/server.crt
#  KEY_FILE      ../ulib/CA/server.key
# PASSWORD       stefano
# CA_PATH        ../ulib/CA/CApath
# CA_FILE        ../ulib/CA/cacert.pem
# VERIFY_MODE    1

  PREFORK_CHILD  3
}

# ------------------------------------------------------------------------------------------------------------------------------------------
# mod_http - plugin parameters
# ------------------------------------------------------------------------------------------------------------------------------------------
# ALIAS                      vector of URI redirection (request -> alias)
# REWRITE_RULE_NF            vector of URI rewrite rule applied after checks that files do not exist (regex1 -> uri1 ...)
#
# CACHE_FILE_MASK            mask (DOS regexp) of pathfile that be cached in memory
#
# VIRTUAL_HOST               flag to activate practice of maintaining more than one server on one machine,
#                            as differentiated by their apparent hostname 
# DIGEST_AUTHENTICATION      flag authentication method (yes = digest, no = basic)
#
# URI_PROTECTED_MASK         mask (DOS regexp) of URI protected from prying eyes
# URI_PROTECTED_ALLOWED_IP   list of comma separated client address for IP-based access control (IPADDR[/MASK]) for URI_PROTECTED_MASK
#
# URI_REQUEST_CERT_MASK      mask (DOS regexp) of URI where client must comunicate a certificate in the SSL connection
#
# This directive gives greater control over abnormal client request behavior, which may be useful for avoiding some forms of denial-of-service attacks
#
# LIMIT_REQUEST_BODY			  restricts the total size of the HTTP request body sent from the client
# REQUEST_READ_TIMEOUT       set timeout for receiving requests
# ------------------------------------------------------------------------------------------------------------------------------------------

mod_http {

   # Allows you to tell clients about documents that used to exist in your server's namespace, but do not anymore.
   # The client will make a request for the document at its new location
 
#  ALIAS [
#	      /login  /RA/cgi-bin/login.sh
#        /admin  /RA/admin/cgi-bin/card-generation.sh
#        ]

   # vector of URI rewrite rule applied after checks that files do not exist (regex1 -> uri1 ...)

#	REWRITE_RULE_NF [
#                  ^/.*\.[A-Za-z0-9]+.*?$	$0
#                  ^/(.*?)(\?|$)(.*)      /sapphire/main.php?url=$1&$3
#                  ]

# CACHE_FILE_MASK       *.css|*.js|*.gif|*.*html

# VIRTUAL_HOST          yes
# DIGEST_AUTHENTICATION yes
#
# URI_PROTECTED_MASK        /RA/admin/cgi-bin/*
# URI_PROTECTED_ALLOWED_IP  127.0.0.1,10.30.0.0/16
#
# URI_REQUEST_CERT_MASK     /wi-auth/cgi-bin/cpe.sh

# This directive gives greater control over abnormal client request behavior,
# which may be useful for avoiding some forms of denial-of-service attacks
#
# LIMIT_REQUEST_BODY			  100K
# REQUEST_READ_TIMEOUT       30 
}

# -----------------------------------------------------------------------------------------------
# mod_fcgi - plugin parameters
# -----------------------------------------------------------------------------------------------
# FCGI_URI_MASK  mask (DOS regexp) of uri type that send request via FCGI (*.php)
#
# SOCKET_NAME    name file for the listening socket
#
# USE_IPV6       flag to indicate use of ipv6
# SERVER         host name or ip address for server
# PORT           port number for the server
#
# RES_TIMEOUT    timeout for response from server FCGI
# FCGI_KEEP_CONN If not zero, the server FCGI does not close the connection after
#                responding to request; the plugin retains responsibility for the connection
#
#
# LOG_FILE       location for file log (use server log if same value)
# -----------------------------------------------------------------------------------------------
#
# mod_fcgi {
#
# FCGI_URI_MASK *.php
#
# SOCKET_NAME   tmp/fcgi.socket
#
# SERVER        127.0.0.1
# PORT          8080
#
# RES_TIMEOUT    20
# FCGI_KEEP_CONN yes
#
# LOG_FILE      /var/log/userver.log
# }

# -----------------------------------------------------------------------------------------------
# mod_scgi - plugin parameters
# -----------------------------------------------------------------------------------------------
# SCGI_URI_MASK  mask (DOS regexp) of uri type that send request via SCGI (*.py)
#
# SOCKET_NAME    name file for the listening socket
#
# USE_IPV6       flag to indicate use of ipv6
# SERVER         host name or ip address for server
# PORT           port number for the server
#
# RES_TIMEOUT    timeout for response from server SCGI
# SCGI_KEEP_CONN If not zero, the server SCGI does not close the connection after
#                responding to request; the plugin retains responsibility for the connection
#
#
# LOG_FILE       location for file log (use server log if same value)
# -----------------------------------------------------------------------------------------------
#
# mod_scgi {
#
# SCGI_URI_MASK *.php
#
# SOCKET_NAME   tmp/scgi.socket
#
# SERVER        127.0.0.1
# PORT          8080
#
# RES_TIMEOUT    20
# SCGI_KEEP_CONN yes
#
# LOG_FILE      /var/log/userver.log
# }

# -----------------------------------------------------------------------------------------------------
# mod_ssi - plugin parameters
# -----------------------------------------------------------------------------------------------------
# SSI_EXT_MASK  mask (DOS regexp) of special filename extension that recognize an SSI-enabled HTML file
# -----------------------------------------------------------------------------------------------------

# mod_ssi {
#
# SSI_EXT_MASK *.shtml|*.shtm|*.sht 
# }

# -----------------------------------------------------------------------------------------------
# mod_tsa - plugin parameters
# -----------------------------------------------------------------------------------------------
# COMMAND      command to execute
# ENVIRONMENT  environment for command to execute
# -----------------------------------------------------------------------------------------------

# mod_tsa {

   # ENV[HOME]         = Base directory for op
   # ENV[OPENSSL]      = Openssl path
   # ENV[OPENSSL_CNF]  = Openssl configuration
   # ENV[ENGINE]       = Openssl Engine to use
   # ENV[PASSWORD]     = Password for key decryption
   # ENV[TSA_CACERT]   = TSA CA chain certificate
   # ENV[TSA_CERT]     = TSA certificate
   # ENV[TSA_KEY]      = TSA private key
   # ENV[FILE_LOG]     = Log file for command
   # ENV[MSG_LOG]      = Log separator
   # ENV[TMPDIR]       = Temporary directory
   # ENV[DEBUG]        = Enable debugging

#  ENVIRONMENT  "HOME=TSA \
#                OPENSSL=bin/openssl \
#                OPENSSL_CNF=CA/openssl.cnf \
#                TSA_CACERT=CA/cacert.pem \
#                TSA_CERT=CA/server.crt \
#                TSA_KEY=CA/server.key"

   # ARGV[1] = TSA REQUEST
   # ARGV[2] = TOKEN
   # ARGV[3] = SEC
   # ARGV[3] = POLICY

#  COMMAND  TSA/TSA_command/tsa_REPLY_BIN.sh
# }

# ------------------------------------------------------------------------------------------------------------------------------------------------
# mod_stream - plugin parameters
# ------------------------------------------------------------------------------------------------------------------------------------------------
# COMMAND                      command to execute
# ENVIRONMENT  environment for command to execute
#
# URI_PATH     specifies the local part of the URL path at which you would like the content to appear (Ex. /my/video.ogv)
# METADATA     specifies the needs to have setup headers prepended for each codec stream (Ex. /my/audio.ogg)
# CONTENT_TYPE specifies the Internet media type of the stream, which will appear in the Content-Type HTTP response header
# ------------------------------------------------------------------------------------------------------------------------------------------------

# mod_stream {

#  ENVIRONMENT   "\"UTRACE=0 5M 0\""
#  COMMAND       my_stream.sh

#  URI_PATH      /my/stream
#  METADATA      /tmp/metadata
#  CONTENT_TYPE  text/plain
#  CONTENT_TYPE  "multipart/x-mixed-replace; boundary=++++++++"
# }

# ------------------------------------------------------------------------------------------------------------------------------------------------
# mod_socket - plugin parameters
# ------------------------------------------------------------------------------------------------------------------------------------------------
# COMMAND                      command to execute
# ENVIRONMENT  environment for command to execute
#
# USE_SIZE_PREAMBLE  use last specification (http://www.whatwg.org/specs/web-socket-protocol/)
# ------------------------------------------------------------------------------------------------------------------------------------------------
#
# mod_socket {
#
#  ENVIRONMENT   "\"UTRACE=0 5M 0\""
#  COMMAND       my_websocket.sh
#
#  USE_SIZE_PREAMBLE  yes
# }

# ------------------------------------------------------------------------------------------------------
# mod_rpc - plugin parameters
# ------------------------------------------------------------------------------------------------------
# METHOD_NAME    name of method
# COMMAND        command to execute
# ENVIRONMENT    environment for command to execute
# RESPONSE_TYPE  input/output type of the command (      success_or_failure     |
#                                                  stdin_success_or_failure     |
#                                                        standard_output        |
#                                                  stdin_standard_output        |
#                                                        standard_output_binary |
#                                                  stdin_standard_output_binary )
# ------------------------------------------------------------------------------------------------------

# mod_rpc  {

   # SOAP or RPC services
   # ------------------------------------------------------------------------------------------------------
   # ENV[HOME]      = Base directory for op
   # ENV[FILE_LOG]  = Log file for command
   # ENV[MSG_LOG]   = Log separator
   # ENV[DEBUG]     = Enable debugging
   # ------------------------------------------------------------------------------------------------------

#  Method_01 {

   # activate a card and send back login/pwd via SMS
   # SMS from client = stringa "From: xxxx\nTo: xxxxx..." input

#  METHOD_NAME CARD

   # ENV[LDAP_HOST]     = LDAP Host
   # ENV[LDAP_PASSWORD] = file contenente password for LDAP binding
   # ENV[MAIL_TO]       = Email Address for workflow
   # ENV[MAIL_FROM]     = Email Address for workflow

#  ENVIRONMENT    "HOME=WAGSM \
#                  DEBUG=1 \
#                  LDAP_HOST=rosso \
#                  LDAP_PASSWORD=unwired-portal_rootdn.pw \
#                  MAIL_TO=card-activation@auth.t-unwired.com \
#                  MAIL_FROM=gsmbox@auth.t-unwired.com"

#  COMMAND        WAGSM/WAGSM_command/card_activation.sh

#  RESPONSE_TYPE  standard_output
#  }
# }

# ------------------------------------------------------------------------------------------------------
# mod_soap - plugin parameters
# ------------------------------------------------------------------------------------------------------
# METHOD_NAME    name of method
# COMMAND        command to execute
# ENVIRONMENT    environment for command to execute
# RESPONSE_TYPE  input/output type of the command (      success_or_failure     |
#                                                  stdin_success_or_failure     |
#                                                        standard_output        |
#                                                  stdin_standard_output        |
#                                                        standard_output_binary |
#                                                  stdin_standard_output_binary )
# ------------------------------------------------------------------------------------------------------

# mod_soap {

   # SOAP or RPC services
   # ------------------------------------------------------------------------------------------------------
   # ENV[HOME]      = Base directory for op
   # ENV[FILE_LOG]  = Log file for command
   # ENV[MSG_LOG]   = Log separator
   # ENV[DEBUG]     = Enable debugging
   # ------------------------------------------------------------------------------------------------------

#  Method_01 {

   # activate a card and send back login/pwd via SMS
   # SMS from client = stringa "From: xxxx\nTo: xxxxx..." input

#  METHOD_NAME CARD

   # ENV[LDAP_HOST]     = LDAP Host
   # ENV[LDAP_PASSWORD] = file contenente password for LDAP binding
   # ENV[MAIL_TO]       = Email Address for workflow
   # ENV[MAIL_FROM]     = Email Address for workflow

#  ENVIRONMENT    "HOME=WAGSM \
#                  DEBUG=1 \
#                  LDAP_HOST=rosso \
#                  LDAP_PASSWORD=unwired-portal_rootdn.pw \
#                  MAIL_TO=card-activation@auth.t-unwired.com \
#                  MAIL_FROM=gsmbox@auth.t-unwired.com"

#  COMMAND        WAGSM/WAGSM_command/card_activation.sh

#  RESPONSE_TYPE  standard_output
#  }
# }

# ---------------------------------------------------------------------------------------------------------------------------------
# mod_proxy - plugin parameters
# ---------------------------------------------------------------------------------------------------------------------------------
# ERROR MESSAGE      Allows you to tell clients about what type of error occurred
#
# REPLACE_RESPONSE   if NOT manage to follow redirects, vector of substitution string
#
# URI                uri trigger
# HOST               name host client
# METHOD_NAME        what type of HTTP method is considered (GET|HEAD|POST)
# CLIENT_CERTIFICATE if yes client must comunicate a certificate in the SSL connection
# PORT               port of server for connection
# SERVER             name of server for connection
# COMMAND            command to execute
# ENVIRONMENT        environment for command to execute
# RESPONSE_TYPE      output type of the command (yes = response for client, no = request to server)
# FOLLOW_REDIRECTS   if yes manage to automatically follow redirects from server
# USER               if       manage to follow redirects, in response to a HTTP_UNAUTHORISED response from the HTTP server: user
# PASSWORD           if       manage to follow redirects, in response to a HTTP_UNAUTHORISED response from the HTTP server: password
# ---------------------------------------------------------------------------------------------------------------------------------

# mod_proxy {

	# ---------------------------------------------------------------------------
	# Allows you to tell clients about what type of error occurred
	# ---------------------------------------------------------------------------
	# ERROR MESSAGE [
	#               STRING   "HTTP/1.1 400 Bad Request\r\n\
	#                         Date: %D\r\n\
	#                         Server: ULib/1.0\r\n\
	#                         Connection: close\r\n\
	#                         Content-Type: text/html; charset=iso-8859-1\r\n\
	#                         Content-Length: 238\r\n\
	#                         \r\n\
	#                         <!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\r\n\
	#                         <html><head>\r\n\
	#                         <title>Accesso negato</title>\r\n\
	#                         </head><body>\r\n\
	#                         <h1>Bad Request</h1>\r\n\
	#                         <p>Errore parsing header<br />\r\n\ </p>\r\n\
	#                         <hr>\r\n\
	#                         <address>ULib Server (mod_proxy)</address>\r\n\
	#                         </body></html>\r\n"
	#
	#               ## ERROR_A_BAD_HEADER
	#               ## ERROR_A_X509_MISSING_CRT
	#               ## ERROR_A_X509_REJECTED
	#               FILE      error_msg/X509_REJECTED
	#               ## ERROR_A_X509_TAMPERED
	#               ## ERROR_A_X509_NOBASICAUTH
	#               ]

	# Service_WAYF {
	#  URI         ^/(WAYF/?|SWITCHaai/images/.*\.gif$)
	#  HOST        localhost:444
	#  METHOD_NAME GET|POST
	#  CLIENT_CERTIFICATE yes
	#  PORT     80
	#  SERVER   localhost
	#  la redirect chiede di nuovo al proxy (444)...!!!
	#  FOLLOW_REDIRECTS  no
	# }
	#
	# Service_IDP {
	#  URI         ^/shibboleth-idp/SSO
	#  HOST        localhost:444
	#  METHOD_NAME GET
	#  PORT     80
	#  SERVER   laptop
	#  FOLLOW_REDIRECTS yes
	#  In response to a HTTP_UNAUTHORISED response from the HTTP server,
	#  obtain a user and password for the scheme/realm returned from the HTTP server
	#  USER     user01
	#  PASSWORD stefano1
	# }
	#
	# Service_webmail {
	#  only if NOT manage to follow redirects
	#  REPLACE_RESPONSE [
	#                   "^Location: http:" "Location: https:"
	#                   ]
	#  URI         /webmail
	#  HOST        localhost
	#  METHOD_NAME GET|POST
	#  PORT     80
	#  SERVER   ca.eraclito.unirel.test
	# }
# }

# ------------------------------------------------------------------------------------------------------------------------------------------
# mod_geoip - plugin parameters
# ------------------------------------------------------------------------------------------------------------------------------------------
# COUNTRY_FORBIDDEN_MASK  mask (DOS regexp) of GEOIP country code that give forbidden access
# ------------------------------------------------------------------------------------------------------------------------------------------

# mod_geoip {

# COUNTRY_FORBIDDEN_MASK CN|JP
# }

# --------------------------------------------------------------------------------------------------------------------
# mod_nocat - plugin parameters
# --------------------------------------------------------------------------------------------------------------------
#
# FIREWALL OPTIONS (8 + 2): params for setup the firewall rules, write data to /tmp/firewall.opt
# ********************************************************************************************************************
# ROUTE_ONLY, DNS_ADDR, INCLUDE_PORTS, EXCLUDE_PORTS,
# ALLOWED_WEB_HOSTS, EXTERNAL_DEVICE, INTERNAL_DEVICE, LOCAL_NETWORK
# ********************************************************************************************************************
#
# AUTH_SERVICE_URL  URL to the login service at the authservice. Must be set to the address of your authentication service
# LOGOUT_URL        URL to redirect user after logout
#
# DECRYPT_CMD       PGP command stuff
# DECRYPT_KEY       DES3 password stuff
#
# INIT_CMD          shell commands to  init          the firewall
# RESET_CMD         shell commands to reset          the firewall
# ACCESS_CMD        shell commands to open and close the firewall
#
# LOGIN_TIMEOUT     Number of seconds after a client last login/renewal to terminate their connection
# CHECK_BY_ARPING   metodo aggiuntivo per verificare la presenza di un peer nella tabella ARP (yes -> abilitato)
# --------------------------------------------------------------------------------------------------------------------

# mod_nocat {

   ## ********************************************************************************************************************
   ## FIREWALL OPTIONS (8 + 2): Allows you to tell the params to setup the default firewall rules  
   ## ********************************************************************************************************************
   # Required only if you DO NOT want your gateway to act as a NAT. Give this only if you are running a strictly routed
   # network, and do not need the gateway to enable NAT for you
#  ROUTE_ONLY 1

   # If you choose not to run DNS on your internal network, specify the address(es) of one or more domain name server
   # on the Internet that wireless clients can use to get out. Should be the same DNS that your DHCP server hands out
#  DNS_ADDR	  192.168.210.254

   # Specify TCP ports to allow access to when public class users login. All others will be denied
#  INCLUDE_PORTS "22 80 443"

   # Specify TCP ports to denied access to when public class users login. All others will be allowed. Note that you should
   # use either IncludePorts or ExcludePorts, but not both. If neither is specified, access is granted to all ports to public
   # class users. You should *always* exclude port 25, unless you want to run an portal for wanton spam sending. Users should
   # have their own way of sending mail. It sucks, but that is the way it is.
	#  Comment this out *only if* you are using INCLUDE_PORTS instead
#  EXCLUDE_PORTS "23 25 111"

   # List any domains that you would like to allow web access (TCP port 80 and 443) BEFORE logging in (this is the
   # pre-skip stage, so be careful about what you allow
#  ALLOWED_WEB_HOSTS "159.213.247.45 159.213.248.2"

   ## NETWORK PARAMS (autodetected if not specified)

   # the interface connected to the Internet. Usually eth0 or eth1 under Linux, or maybe even ppp0 if you are running PPP or PPPoE
#  EXTERNAL_DEVICE eth0

   # Required if and only if your machine has more than two network interfaces. Must be set to the interface connected to your local
   # network, normally your wireless card
#  INTERNAL_DEVICE "ath0 ath1"

   # Must be set to the network address and net mask of your internal network. You can use the number of bits in the netmask
   # (e.g. /16, /24, etc.) or the full x.x.x.x specification
#  LOCAL_NETWORK "192.168.210.0/24 192.168.211.0/24"

   ## AUTOMATIC PARAMS ADDED BY THIS PLUGIN

   #  9 GatewayPort     The TCP port to bind the gateway service to. 5280 is de-facto standard for NoCatAuth
   # 10 AuthServiceAddr the address of your authentication service.
   ## ********************************************************************************************************************

   # URL to the login service at the authservice. Must be set to the address of your authentication service
   # You must use an IP address if DNS resolution is not available at gateway startup
#  AUTH_SERVICE_URL   "http://10.30.1.131/"
#  AUTH_SERVICE_URL   "https://94.138.39.147/"

   # URL to redirect user after logout
#  LOGOUT_URL         "http://10.30.1.131/logout"
#  LOGOUT_URL         "https://94.138.39.147/logout"

   # Number of seconds after a client last login/renewal to terminate their connection
   # Probably do not want to set this to less than 60 or a lot of bandwidth is likely to get consumed by the client renewal attempts
#  LOGIN_TIMEOUT     86400 # one notification per day

   # PGP command stuff
#  DECRYPT_CMD       "/usr/bin/gpg --decrypt --homedir=/usr/share/nocat/pgp --batch --decrypt --no-tty --status-file /tmp/gpg.status -o -"

   # DES3 password stuff
#  DECRYPT_KEY       PASSWORD

   # setup the default firewall rules
#  INIT_CMD          firewall/initialize.fw

   # reset the default firewall rules
#  RESET_CMD         firewall/reset.fw

   # access control script to open and close firewall
   # AUTOMATIC PARAMS(5) ADDED BY THIS PLUGIN: [action(permit|deny) mac ip class(Owner|Member|Public)] rulenum
#  ACCESS_CMD        firewall/access.fw

   # metodo aggiuntivo per verificare la presenza di un peer nella tabella ARP (yes -> abilitato)
#  CHECK_BY_ARPING yes
# }

Benchmarking
------------

    $ ./configure && make
    $ cd tests/examples
    $ ./benchmarking.sh (or hello_world.sh)

Use apachebench

	$ ab -n 100000 -c10 http://127.0.0.1/usp/benchmarking.usp?name=stefano (or)
	$ ab -n 100000 -c10 http://127.0.0.1/usp/hello_world.usp

Quickstart
----------

Take a look at:

    $ ./configure --help
* ......
* --with-ssl              use system      SSL library - [will check /usr /usr/local] [default=yes]
* --with-libz             use system     LIBZ library - [will check /usr /usr/local] [default=yes]
* --with-pcre             use system     PCRE library - [will check /usr /usr/local] [default=yes]
* --with-libuuid          use system  libuuid library - [will check /usr /usr/local] [default=yes]
* --with-magic            use system libmagic library - [will check /usr /usr/local] [default=yes]
* --with-expat            use system    EXPAT library - [will check /usr /usr/local] [default=yes]
* --with-ssh              use system      SSH library - [will check /usr /usr/local]
* --with-curl             use system     cURL library - [will check /usr /usr/local]
* --with-ldap             use system openLDAP library - [will check /usr /usr/local]
* --with-mysql            use system    MySQL library - [will check /usr /usr/local]
* --with-libevent         use system libevent library - [will check /usr /usr/local]
* --with-libxml2          use system  libxml2 library - [will check /usr /usr/local]

if you desire wrapping of some system library installed.

More info
---------

ULib is normally built and installed as a set of shared object libraries and header files. These libraries and headers are installed using directories selected through a "configure" script that has been prepared with automake and autoconf. As such, they should build and install similarly to and in a manner compatible and consistent with most other GNU software. ULib is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version. This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

Comments and suggestions are welcome.

	stefano casazza <stefano.casazza@gmail.com>
