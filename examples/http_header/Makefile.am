## Makefile.am -- Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = ## dist-shar dist-zip

MAINTAINERCLEANFILES	= Makefile.in

EXTRA_DIST = include

DEFAULT_INCLUDES = -I./include -I$(top_builddir)/include

include_includedir = $(includedir)/include

SRC = HttpBaAuthorization.cpp HttpBaWwwAuthenticate.cpp HttpCookie.cpp HttpField.cpp HttpHeader.cpp HttpLocation.cpp \
		HttpRequestHeader.cpp HttpResponseHeader.cpp HttpSetCookie.cpp OtpAuthToken.cpp HttpOtpPostLogin.cpp \
		$(top_builddir)/src/ulib/internal/portable.cpp

if DEBUG
## AM_CXXFLAGS = -DDEBUG_DEBUG
endif

libhttp_la_SOURCES = $(SRC)

## for convenience library
noinst_LTLIBRARIES = libhttp.la

#lib_LTLIBRARIES = libhttp.la
#libhttp_la_LDFLAGS = $(LT_VERSION) -release $(LT_RELEASE)
