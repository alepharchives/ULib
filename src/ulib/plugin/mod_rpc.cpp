// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    mod_rpc.cpp - this is a plugin rpc for userver
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/file_config.h>
#include <ulib/net/rpc/rpc.h>
#include <ulib/plugin/mod_rpc.h>
#include <ulib/net/server/server.h>
#include <ulib/net/rpc/rpc_object.h>
#include <ulib/net/rpc/rpc_parser.h>

U_CREAT_FUNC(URpcPlugIn)

URpcPlugIn::~URpcPlugIn()
{
   U_TRACE_UNREGISTER_OBJECT(0, URpcPlugIn)

   if (URPCMethod::encoder)
      {
      delete rpc_parser;
      delete URPCMethod::encoder;
      delete URPCObject::dispatcher;
      }
}

// Server-wide hooks

int URpcPlugIn::handlerConfig(UFileConfig& cfg)
{
   U_TRACE(0, "URpcPlugIn::handlerConfig(%p)", &cfg)

   // Perform registration of server RPC method

   rpc_parser = U_NEW(URPCParser);

   if (cfg.skip()) URPCObject::loadGenericMethod(&cfg);

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

int URpcPlugIn::handlerInit()
{
   U_TRACE(0, "URpcPlugIn::handlerInit()")

   if (URPCMethod::encoder == 0)
      {
      rpc_parser = U_NEW(URPCParser);

      URPCObject::loadGenericMethod(0);
      }

   U_SRV_LOG_MSG("initialization of plugin success");

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

// Connection-wide hooks

int URpcPlugIn::handlerRead()
{
   U_TRACE(0, "URpcPlugIn::handlerRead()")

   // read the RPC request

   bool is_rpc_msg = URPC::readRPC(UClientImage_Base::socket, *UClientImage_Base::rbuffer);

   // check if close connection... (read() == 0)

   if (UClientImage_Base::socket->isClosed()) U_RETURN(U_PLUGIN_HANDLER_ERROR);
   if (UClientImage_Base::rbuffer->empty())   U_RETURN(U_PLUGIN_HANDLER_AGAIN);

   if (UServer_Base::isLog()) UClientImage_Base::logRequest();

   U_RETURN(is_rpc_msg ? U_PLUGIN_HANDLER_FINISHED : U_PLUGIN_HANDLER_ERROR);
}

int URpcPlugIn::handlerRequest()
{
   U_TRACE(0, "URpcPlugIn::handlerRequest()")

   // process the RPC request

   U_INTERNAL_ASSERT_POINTER(rpc_parser)

   bool bSendingFault;
   UString method = UClientImage_Base::rbuffer->substr(0U, U_TOKEN_NM);

   *UClientImage_Base::wbuffer = rpc_parser->processMessage(method, *URPCObject::dispatcher, bSendingFault);

   U_SRV_LOG_VAR_WITH_ADDR("method %.*S process %s for", U_STRING_TO_TRACE(method), (bSendingFault ? "failed" : "passed"));

   U_RETURN(U_PLUGIN_HANDLER_FINISHED);
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* URpcPlugIn::dump(bool reset) const
{
   *UObjectIO::os << "rpc_parser (URPCParser " << (void*)rpc_parser << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
