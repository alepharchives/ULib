ULib - C++ library
================================

What's this?
--------

ULib is a highly optimized class framework for writing C++ applications. I wrote this framework as my tool for writing applications in various contexts.
It is a result of many years of work as C++ programmer. I think, in my opinion, that its strongest points are simplicity, efficiency and sophisticate
debugging. This framework offers a class foundation that disables language features that consume memory or introduce runtime overhead, such as rtti and
exception handling, and assumes one will mostly be linking applications with other pure C based libraries rather than using the overhead of the standard
C++ library and other similar class frameworks. It include as application example a powerful search engine with relative web interface and a multi purpose
server (plugin oriented) which results, out of John Fremlin accurate investigations (http://john.freml.in/ulib-fast-io-framework), to be one of the faster
web application frameworks for serving small dynamic webpages (and also make easier the usage of shell scripts for CGI application)


Quickstart
--------

Take a look at:

    $ ./configure --help
* ......
* --with-libz             use system     LIBZ library - [will check /usr /usr/local]
* --with-magic            use system libmagic library - [will check /usr /usr/local]
* --with-pcre             use system     PCRE library - [will check /usr /usr/local]
* --with-expat            use system    EXPAT library - [will check /usr /usr/local]
* --with-ssl              use system      SSL library - [will check /usr /usr/local]
* --with-ssh              use system      SSH library - [will check /usr /usr/local]
* --with-curl             use system     cURL library - [will check /usr /usr/local]
* --with-ldap             use system openLDAP library - [will check /usr /usr/local]
* --with-libevent         use system libevent library - [will check /usr /usr/local]
* --with-libuuid          use system  libuuid library - [will check /usr /usr/local]

if you desire wrapping of some system library installed.

Benchmarking
--------

    $ ./configure && make
    $ cd tests/examples
    $ ./benchmarking.sh

Use apachebench

	$ ab -n 100000 -c10 http://127.0.0.1/usp/benchmarking.usp?name=stefano

More info
--------

ULib is normally built and installed as a set of shared object libraries and header files. These libraries and headers are installed using directories selected through a "configure" script that has been prepared with automake and autoconf. As such, they should build and install similarly to and in a manner compatible and consistent with most other GNU software. ULib is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version. This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

Comments and suggestions are welcome.

	stefano casazza <stefano.casazza@gmail.com>
