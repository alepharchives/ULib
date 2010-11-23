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
#include <ulib/plugin/mod_echo.h>
#include <ulib/net/server/server.h>
#include <ulib/utility/socket_ext.h>
#include <ulib/net/server/client_image.h>

#ifdef HAVE_MODULES
U_CREAT_FUNC(mod_echo, UEchoPlugIn)
#endif

// Server-wide hooks

// Connection-wide hooks

int UEchoPlugIn::handlerREAD()
{
   U_TRACE(0, "UEchoPlugIn::handlerREAD()")

   bool is_msg = USocketExt::read(UClientImage_Base::socket, *UClientImage_Base::rbuffer);

   // check if close connection... (read() == 0)

   if (UClientImage_Base::socket->isClosed()) U_RETURN(U_PLUGIN_HANDLER_ERROR);
   if (UClientImage_Base::rbuffer->empty())   U_RETURN(U_PLUGIN_HANDLER_AGAIN);

   if (UServer_Base::isLog()) UClientImage_Base::logRequest();

   if (is_msg)
      {
      USocketExt::size_message = UClientImage_Base::rbuffer->size();

      U_RETURN(U_PLUGIN_HANDLER_FINISHED);
      }

   U_RETURN(U_PLUGIN_HANDLER_ERROR);
}

int UEchoPlugIn::handlerRequest()
{
   U_TRACE(0, "UEchoPlugIn::handlerRequest()")

   U_INTERNAL_ASSERT_MAJOR(USocketExt::size_message,0U)

   *UClientImage_Base::wbuffer = UClientImage_Base::rbuffer->substr(0U, USocketExt::size_message);

    UClientImage_Base::body->clear();

   U_RETURN(U_PLUGIN_HANDLER_FINISHED);
}
