// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    mod_soap.cpp - this is a plugin soap for userver
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/file_config.h>
#include <ulib/utility/uhttp.h>
#include <ulib/net/server/server.h>
#include <ulib/xml/soap/soap_object.h>
#include <ulib/xml/soap/soap_parser.h>
#include <ulib/net/server/plugin/mod_soap.h>

#ifdef HAVE_MODULES
U_CREAT_FUNC(mod_soap, USoapPlugIn)
#endif

USOAPParser* USoapPlugIn::soap_parser;

USoapPlugIn::USoapPlugIn()
{
   U_TRACE_REGISTER_OBJECT_WITHOUT_CHECK_MEMORY(0, USoapPlugIn, "")
}

USoapPlugIn::~USoapPlugIn()
{
   U_TRACE_UNREGISTER_OBJECT(0, USoapPlugIn)

   if (soap_parser)
      {
      delete soap_parser;
      delete URPCMethod::encoder;
      delete URPCObject::dispatcher;
      }
}

// Server-wide hooks

int USoapPlugIn::handlerConfig(UFileConfig& cfg)
{
   U_TRACE(0, "USoapPlugIn::handlerConfig(%p)", &cfg)

   // Perform registration of server SOAP method

   soap_parser = U_NEW(USOAPParser);

   USOAPObject::loadGenericMethod(&cfg);

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

int USoapPlugIn::handlerInit()
{
   U_TRACE(0, "USoapPlugIn::handlerInit()")

   if (soap_parser)
      {
      U_SRV_LOG("initialization of plugin success");

      // NB: SOAP is NOT a static page...

      if (UHTTP::valias == 0) UHTTP::valias = U_NEW(UVector<UString>(2U));

      UHTTP::valias->push_back(U_STRING_FROM_CONSTANT("/soap"));
      UHTTP::valias->push_back(U_STRING_FROM_CONSTANT("/nostat"));

      goto end;
      }

   U_SRV_LOG("initialization of plugin FAILED");

end:
   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

// Connection-wide hooks

int USoapPlugIn::handlerRequest()
{
   U_TRACE(0, "USoapPlugIn::handlerRequest()")

   if (soap_parser &&
       UHTTP::isSOAPRequest())
      {
      // process the SOAP message -- should be the contents of the message from "<SOAP:" to the end of the string

      bool bSendingFault;

      UString body   = soap_parser->processMessage(*UClientImage_Base::body, *URPCObject::dispatcher, bSendingFault),
              method = soap_parser->getMethodName();

      U_SRV_LOG_WITH_ADDR("method %.*S process %s for", U_STRING_TO_TRACE(method), (bSendingFault ? "failed" : "passed"));

#  ifdef DEBUG
      (void) UFile::writeToTmpl("/tmp/soap.res", body);
#  endif

      u_http_info.nResponseCode  = HTTP_OK;

      UHTTP::setResponse(UHTTP::str_ctype_soap, &body);

      UHTTP::setRequestProcessed();
      }

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* USoapPlugIn::dump(bool reset) const
{
   *UObjectIO::os << "soap_parser (USOAPParser " << (void*)soap_parser << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
