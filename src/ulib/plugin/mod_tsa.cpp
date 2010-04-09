// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    mod_tsa.cpp - this is a plugin tsa for userver
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/command.h>
#include <ulib/file_config.h>
#include <ulib/utility/uhttp.h>
#include <ulib/plugin/mod_tsa.h>
#include <ulib/net/server/server.h>

U_CREAT_FUNC(UTsaPlugIn)

UTsaPlugIn::~UTsaPlugIn()
{
   U_TRACE_UNREGISTER_OBJECT(0, UTsaPlugIn)

   if (command) delete command;
}

// Server-wide hooks

int UTsaPlugIn::handlerConfig(UFileConfig& cfg)
{
   U_TRACE(0, "UTsaPlugIn::handlerConfig(%p)", &cfg)

   // -----------------------------------------------
   // Perform registration of userver method
   // -----------------------------------------------
   // COMMAND      command to execute
   // ENVIRONMENT  environment for command to execute
   // -----------------------------------------------

   if (cfg.loadTable()) command = UServer_Base::loadConfigCommand(cfg);

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

int UTsaPlugIn::handlerInit()
{
   U_TRACE(0, "UTsaPlugIn::handlerInit()")

   if (command)
      {
      U_SRV_LOG_MSG("initialization of plugin success");

      U_RETURN(U_PLUGIN_HANDLER_GO_ON);
      }

   U_SRV_LOG_MSG("initialization of plugin FAILED");

   U_RETURN(U_PLUGIN_HANDLER_ERROR);
}

// Connection-wide hooks

int UTsaPlugIn::handlerRequest()
{
   U_TRACE(0, "UTsaPlugIn::handlerRequest()")

   if (UHTTP::isTSARequest() == false) U_RETURN(U_PLUGIN_HANDLER_GO_ON);

   // process TSA request

   U_INTERNAL_ASSERT_POINTER(command)

   if (command->execute(UClientImage_Base::body, UClientImage_Base::wbuffer) == false)
      {
      UServer_Base::logCommandMsgError(command->getCommand());

      UHTTP::setHTTPInternalError(); // set internal error response...

      goto end;
      }

   *UClientImage_Base::wbuffer = UHTTP::getHTTPHeaderForResponse(HTTP_OK, "application/timestamp-reply", *UClientImage_Base::wbuffer);

end:

   U_RETURN(U_PLUGIN_HANDLER_FINISHED);
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UTsaPlugIn::dump(bool reset) const
{
   *UObjectIO::os << "command (UCommand " << (void*)command << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
