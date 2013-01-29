// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    mod_echo.cpp - this is plugin ECHO for userver
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/file_config.h>
#include <ulib/utility/uhttp.h>
#include <ulib/net/server/server.h>
#include <ulib/utility/socket_ext.h>
#include <ulib/net/server/client_image.h>
#include <ulib/net/server/plugin/mod_echo.h>

#ifdef HAVE_MODULES
U_CREAT_FUNC(mod_echo, UEchoPlugIn)
#endif

/*
#define U_RESPONSE_FOR_TEST \
"HTTP/1.0 200 OK\r\n" \
"Server: ULib\r\n" \
"Connection: close\r\n" \
"Content-Type: text/html\r\n" \
"Content-Length: 22\r\n\r\n" \
"<h1>Hello stefano</h1>"
*/

// Server-wide hooks

// Connection-wide hooks

int UEchoPlugIn::handlerRequest()
{
   U_TRACE(0, "UEchoPlugIn::handlerRequest()")

#ifdef U_RESPONSE_FOR_TEST
   UClientImage_Base::wbuffer->assign(U_CONSTANT_TO_PARAM(U_RESPONSE_FOR_TEST));
#else
   *UClientImage_Base::wbuffer = *UClientImage_Base::request;
#endif

   UClientImage_Base::body->clear();

   UHTTP::endRequestProcessing();

   U_RETURN(U_PLUGIN_HANDLER_FINISHED);
}
