ULib - C++ library
==================

ULib is a highly optimized class framework for writing C++ applications. I wrote this framework as my tool for writing applications in various contexts.
It is a result of many years of work as C++ programmer. I think, in my opinion, that its strongest points are simplicity, efficiency and sophisticate
debugging. This framework offers a class foundation that disables language features that consume memory or introduce runtime overhead, such as rtti and
exception handling, and assumes one will mostly be linking applications with other pure C based libraries rather than using the overhead of the standard
C++ library and other similar class frameworks. It include as application example a powerful search engine with relative web interface and a multi purpose
server (plugin oriented) which results, out of [John Fremlin accurate investigations](http://john.freml.in/ulib-fast-io-framework), to be one of the faster
web application frameworks for serving small dynamic webpages (and also make easier the usage of shell scripts for CGI application)


userver_(tcp|ssl|ipc) application server (plugin oriented)
----------------------------------------------------------

The current version offers the following features :

   * HTTP/1.0 and 1.1 protocols supported.
   * Persistent connections for HTTP/1.1 and Keep-Alive support for HTTP/1.0.
   * Browser cache management (headers: If-Modified-Since/Last-modified).
   * Chunk-encoding transfers support.
   * HTTP multi-range request support.
   * Memory caching of document root for (small) static pages with smart compression and CSS/JS reduction.
   * Support for automatic update of caching document root with inotify (on Linux).
   * Support for pipelining.
   * Support for virtual hosts (also with SSL).
   * Support for basic/digest authentication.
   * Support for directory listings via basic/digest authentication.
   * Support for uri protection.
   * Support for aliases/redirection.
   * Support for RewriteRule (lighttpd-like) that check for file existence as they do on Apache,
     some CMS (SilverStripe) require it.
   * Support for [JSONRequest](http://json.org/JSONRequest.html).
   * Accept HTTP uploads up to 4 GB without increasing memory usage.
   * Support for upload progress via USP (ULib Servlet Page).
   * General [CGI](http://it.wikipedia.org/wiki/Common_Gateway_Interface) support (run any CGI script) with automatic output compression (using deflate method).
   * CGI support for shell script processes (with automatic management of form and cookie).
   * CGI support for the X-Sendfile feature and also supports X-Accel-Redirect headers transparently.
   * Support for minify HTML CGI output with wrapping [google page speed SDK](http://code.google.com/speed/page-speed/download.html#pagespeed-sdk).
   * Support for Windows (without preforking).
   * Requests cut in phases for modular architecture (apache-like).
   * Configuration file with dedicated section.
   * Built-in modules :
       * mod_echo : echo features.
       * mod_rpc : generic Remote Procedure Call.
       * mod_http : core features, static file handler and dynamic page (ULib Servlet Page).
       * mod_ssi : [Server Side Includes]( http://en.wikipedia.org/wiki/Server_Side_Include) support with enhanced #set,
		             direct include and #exec usp (ULib Servlet Page) and csp (C Servlet Page).
       * mod_nocat : [captive portal](http://nocat.net/) implementation.
       * mod_tsa : server side [Time Stamp](http://www.opentsa.org) support.
       * mod_soap : generic [SOAP](http://java.sun.com/developer/technicalArticles/xml/webservices) server services support.
       * mod_fcgi : third-party applications support thru [FastCGI](http://www.fastcgi.com/drupal) interface.
       * mod_scgi : module that implements the client side of the [SCGI](http://www.mems-exchange.org/software/scgi) protocol (experimental).
       * mod_shib : [web single sign-on support](http://shibboleth.internet2.edu) (experimental).
       * mod_proxy : proxy support (experimental).
       * mod_tcc : [C Servlet Support](http://bellard.org/tcc/) with libtcc as a backend for dynamic code generation (experimental).
       * mod_geoip : [geolocation support](http://www.maxmind.com/geoip/api/c.shtml) (experimental).
       * mod_stream : simple streaming support (experimental).
       * mod_socket : [Web Socket](http://dev.w3.org/html5/websockets) application framework (experimental).


Benchmarking
------------

    $ ./configure && make
    $ cd tests/examples
    $ ./benchmarking.sh (or hello_world.sh)

Use apachebench (ab)

	$ ab -n 100000 -c10 http://127.0.0.1/usp/benchmarking.usp?name=stefano (or)
	$ ab -n 100000 -c10 http://127.0.0.1/usp/hello_world.usp


[Comparative Benchmarking](https://github.com/stefanocasazza/ULib/tree/master/doc/benchmark)
--------------------------------------------------------------------------------------------

I consider in this benchmark only the performant server [G-WAN 2.1.20 (32 bit)] (http://www.gwan.ch/).
All tests are performed on an Intel Pentium 4 2.8 Ghz, Hard drive 5400 rpm, Memory: 2GB DDR2 800MHz) running Gentoo 64 bit (kernel 2.6.38.2).

The client [bench1.c](https://github.com/stefanocasazza/ULib/tree/master/doc/benchmark/bin/bench1.c)
relies on ApacheBench (ab) and it is a slightly modified version of [G-WAN client](http://gwan.ch/source/ab.c.txt).

The client is running on different computer than the web server (networking is involved).

I had to increase the local port range on client (because of the TIME_WAIT status of the TCP ports).

 * HTTP Keep-Alives: yes/no
 * Concurrency: from 0 to 1000, step 10
 * Requests: up to 1000000 - within a fixed total amount of time (1 sec)

For serving static content I use 3 file of different size:

 *   99.html         (  99 byte)
 * 1000.html	      (1000 byte)
 * WebSocketMain.swf (180K byte)

For serving dynamic content I use a simple request: "Hello {name}"

The raw data in csv format are [here](https://github.com/stefanocasazza/ULib/tree/master/doc/benchmark).

userver_tcp is the winner of this benchmark in all case for almost all level of concurrency.


Quickstart
----------

Take a look at:

    $ ./configure --help
* ......
* --with-libz             use system     LIBZ library - [will check /usr /usr/local] [default=yes]
* --enable-zip            enable build of ZIP support - require libz [default: depend from libz]
* --with-libuuid          use system  libuuid library - [will check /usr /usr/local] [default=yes]
* --with-magic            use system libmagic library - [will check /usr /usr/local] [default=yes]
* --with-ssl              use system      SSL library - [will check /usr /usr/local] [default=yes]
* --with-ssh              use system      SSH library - [will check /usr /usr/local]
* --with-pcre             use system     PCRE library - [will check /usr /usr/local] [default=yes]
* --with-expat            use system    EXPAT library - [will check /usr /usr/local] [default=yes]
* --with-curl             use system     cURL library - [will check /usr /usr/local]
* --with-ldap             use system openLDAP library - [will check /usr /usr/local]
* --with-mysql            use system    MySQL library - [will check /usr /usr/local]
* --with-dbi              use system      DBI library - [will check /usr /usr/local]
* --with-libevent         use system libevent library - [will check /usr /usr/local]
* --with-libxml2          use system  libxml2 library - [will check /usr /usr/local]
* --with-page-speed       use google page-speed SDK   - [will check /usr /usr/local] [default=no]

if you desire wrapping of some system library installed.


More info
---------

ULib is normally built and installed as a set of shared object libraries and header files. These libraries and headers are installed using directories selected through a "configure" script that has been prepared with automake and autoconf. As such, they should build and install similarly to and in a manner compatible and consistent with most other GNU software. ULib is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version. This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

Comments and suggestions are welcome.

	stefano casazza <stefano.casazza@gmail.com>
