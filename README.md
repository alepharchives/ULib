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
    * Memory caching for small static pages with optional compression.
    * Support for pipelining.
    * Support for virtual hosts.
    * Support for basic/digest authentication.
    * Support for uri protection.
    * Support for aliases/redirection.
    * Support for HTTP multi-range request support.
    * Support for JSONRequest (http://json.org/JSONRequest.html).
    * Web Socket support (experimental).
    * Support for Windows (without preforking).
    * Requests cut in phases for modular architecture (Apache-like).
    * Built-in modules :
          o mod_rpc : generic Remote Procedure Call.
          o mod_http : core features, static file handler and dynamic page (ULib Servlet Page).
          o mod_fcgi : third-party applications support thru FastCGI interface.
          o mod_tsa : server side time stamp support.
          o mod_echo : echo features.
          o mod_soap : generic SOAP server services support.
          o mod_nocat : captive portal implementation.
          o mod_shib : web single sign-on support (experimental).
          o mod_proxy : proxy support (experimental).
          o mod_geoip : geolocation support (experimental).
          o mod_stream : simple streaming support (experimental).
          o mod_socket : web sockets application framework (experimental).
    * Configuration file with dedicated section.
    * CGI support for shell script processes (with automatic management of form and cookie).
    * General CGI support (run any CGI script) with automatic output compression (using deflate method).

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
