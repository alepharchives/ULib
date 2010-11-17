## Makefile.am -- Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = no-dependencies
MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = ulib

if PLUGIN
SUBDIRS += ulib/plugin ulib/plugin/usp
if FCGI
if DEBUG
SUBDIRS += ulib/plugin/mod_fcgi
endif
endif
if GEOIP
if !STATIC_HANDLER_GEOIP
SUBDIRS += ulib/plugin/mod_geoip
endif
endif
if SSL
if PCRE
if MOD_SHIB
if !STATIC_HANDLER_SHIB
SUBDIRS += ulib/plugin/mod_shib
endif
endif
endif
endif
endif
