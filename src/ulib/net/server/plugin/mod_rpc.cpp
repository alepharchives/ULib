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
#include <ulib/net/server/server.h>
#include <ulib/net/rpc/rpc_object.h>
#include <ulib/net/rpc/rpc_parser.h>
#include <ulib/net/server/plugin/mod_rpc.h>

#ifdef HAVE_MODULES
U_CREAT_FUNC(mod_rpc, URpcPlugIn)
#endif

URpcPlugIn::~URpcPlugIn()
{
   U_TRACE_UNREGISTER_OBJECT(0, URpcPlugIn)

   if (rpc_parser)
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

   URPCObject::loadGenericMethod(&cfg);

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

int URpcPlugIn::handlerInit()
{
   U_TRACE(0, "URpcPlugIn::handlerInit()")

   if (rpc_parser)
      {
      U_SRV_LOG("initialization of plugin success");

      goto end;
      }

   U_SRV_LOG("initialization of plugin FAILED");

end:
   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

// Connection-wide hooks

int URpcPlugIn::handlerREAD()
{
   U_TRACE(0, "URpcPlugIn::handlerREAD()")

   if (rpc_parser)
      {
      // check for RPC request

      is_rpc_msg = URPC::readRPCRequest(UServer_Base::pClientImage->socket, false); // NB: resetRPCInfo() it is already called by clearData()...

      if (is_rpc_msg) U_RETURN(U_PLUGIN_HANDLER_FINISHED);
      }

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

int URpcPlugIn::handlerRequest()
{
   U_TRACE(0, "URpcPlugIn::handlerRequest()")

   if (is_rpc_msg)
      {
      // process the RPC request

      U_INTERNAL_ASSERT_POINTER(rpc_parser)
      U_ASSERT_EQUALS(UClientImage_Base::request->empty(), false)

      bool bSendingFault;
      UString method = UClientImage_Base::request->substr(0U, U_TOKEN_NM);

      *UClientImage_Base::wbuffer = rpc_parser->processMessage(method, *URPCObject::dispatcher, bSendingFault);

      U_SRV_LOG_WITH_ADDR("method %.*S process %s for", U_STRING_TO_TRACE(method), (bSendingFault ? "failed" : "passed"));

      U_RETURN(U_PLUGIN_HANDLER_FINISHED);
      }

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* URpcPlugIn::dump(bool reset) const
{
   *UObjectIO::os << "is_rpc_msg             " << is_rpc_msg        << '\n'
                  << "rpc_parser (URPCParser " << (void*)rpc_parser << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
