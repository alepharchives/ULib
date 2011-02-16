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
#include <ulib/net/server/server.h>
#include <ulib/utility/socket_ext.h>
#include <ulib/net/server/client_image.h>
#include <ulib/net/server/plugin/mod_echo.h>

#ifdef HAVE_MODULES
U_CREAT_FUNC(mod_echo, UEchoPlugIn)
#endif

// Server-wide hooks

// Connection-wide hooks

int UEchoPlugIn::handlerRequest()
{
   U_TRACE(0, "UEchoPlugIn::handlerRequest()")

   *UClientImage_Base::wbuffer = *UClientImage_Base::request;

    UClientImage_Base::body->clear();

   U_RETURN(U_PLUGIN_HANDLER_FINISHED);
}
