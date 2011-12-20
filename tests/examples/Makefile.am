## Makefile.am -- Process this file with automake to produce Makefile.in

MAINTAINERCLEANFILES	= Makefile.in

DEFAULT_INCLUDES =  -I. -I$(top_srcdir) -I$(top_srcdir)/include -I$(top_srcdir)/examples/http_header/include

EXTRA_DIST = inp ok CA CSP LCSP TSA RSIGN XAdES nocat wi-auth WAGSM RA icons IR/WEB IR/benchmark IR/doc_dir *.cfg *.properties *.test *.sh \
				 index.html calcajax.html .htpasswd .htdigest cgi-bin www.sito?.com error_msg workflow doc_parse favicon.ico robots.txt SSI css js \
				 libraries/libcaptcha.c libraries/Makefile benchmark servlet

DEFS  = -DU_TEST @DEFS@

TESTS = client_server.test test_manager.test IR.test web_server.test web_server_multiclient.test web_socket.test ## workflow.test

if SSL
TESTS += tsa_http.test tsa_https.test csp_rpc.test rsign_rpc.test tsa_rpc.test uclient.test
if EXPAT
TESTS += csp.test tsa_ssoap.test rsign.test
endif
## MINGW don't have unix socket...
if !MINGW
TESTS += lcsp_rpc.test
if EXPAT
TESTS += lcsp.test
endif
endif
endif

if SSL
PRG = test_http_header
test_http_header_SOURCES = test_http_header.cpp
HTTP_LIB = $(top_builddir)/examples/http_header/libhttp.la
TESTS += http_header.test web_server_ssl.test
if LIBZ
TESTS += PEC_report_rejected.test PEC_report_messaggi.test PEC_report_virus.test PEC_report_anomalie.test PEC_check_namefile.test
if ZIP
TESTS += doc_parse.test doc_classifier.test
endif
endif
##if LIBXML2
##TESTS += XAdES.test
##endif
endif

if EXPAT
TESTS += xml2txt.test
endif

## if LDAP
## TESTS += form_completion.test
## if SSL
## if SSH
## TESTS += lrp_session.test lrp_pusher.test
## endif
## endif
## endif

## if USE_PARSER
## TESTS += parser.test
## endif

#if LINUX
#TESTS += download_accelerator.test
#endif

check_PROGRAMS  = $(PRG)
TESTS 			+= ../reset.color

LDADD = $(HTTP_LIB) $(top_builddir)/src/ulib/lib@ULIB@.la

clean-local:
	-rm -rf out err core .libs *.bb* *.da *.gcov trace.*.[0-9]* object.*.[0-9]* stack.*.[0-9]* *.log \
	IR/db* TSA/gSOAP/tsa_client
