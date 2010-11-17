// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    mod_proxy_service.cpp - service for plugin proxy for userver
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef U_MOD_PROXY_SERVICE_CPP
#define U_MOD_PROXY_SERVICE_CPP 1

#include <ulib/date.h>
#include <ulib/command.h>
#include <ulib/file_config.h>
#include <ulib/utility/uhttp.h>
#include <ulib/utility/services.h>
#include <ulib/net/server/server.h>
#include <ulib/utility/string_ext.h>
#include <ulib/plugin/mod_proxy_service.h>

#ifdef HAVE_STRSTREAM_H
#  include <strstream.h>
#else
#  include <ulib/replace/strstream.h>
#endif

const UString* UModProxyService::str_FOLLOW_REDIRECTS;
const UString* UModProxyService::str_CLIENT_CERTIFICATE;

void UModProxyService::str_allocate()
{
   U_TRACE(0, "UModProxyService::str_allocate()")

   U_INTERNAL_ASSERT_EQUALS(str_FOLLOW_REDIRECTS,0)
   U_INTERNAL_ASSERT_EQUALS(str_CLIENT_CERTIFICATE,0)

   static ustringrep stringrep_storage[] = {
      { U_STRINGREP_FROM_CONSTANT("FOLLOW_REDIRECTS") },
      { U_STRINGREP_FROM_CONSTANT("CLIENT_CERTIFICATE") }
   };

   U_NEW_ULIB_OBJECT(str_FOLLOW_REDIRECTS,   U_STRING_FROM_STRINGREP_STORAGE(0));
   U_NEW_ULIB_OBJECT(str_CLIENT_CERTIFICATE, U_STRING_FROM_STRINGREP_STORAGE(1));
}

void UModProxyService::loadConfig(UFileConfig& cfg, UVector<UModProxyService*>& vservice, UVector<UString>* vmsg_error)
{
   U_TRACE(0, "UModProxyService::loadConfig(%p,%p,%p)", &cfg, &vservice, vmsg_error)

   // -----------------------------------------------------------------------------------------------------------------------------------
   // mod_proxy - plugin parameters
   // -----------------------------------------------------------------------------------------------------------------------------------
   // ERROR MESSAGE        Allows you to tell clients about what type of error
   //
   // REPLACE_RESPONSE     if NOT manage to follow redirects, maybe vector of substitution string
   //
   // URI                  uri mask trigger
   // HOST                 name host client
   // METHOD_NAME          mask type of HTTP method is considered (GET|POST)
   // CLIENT_CERTIFICATE   if yes client must comunicate a certificate in the SSL connection
   // PORT                 port of server for connection
   // SERVER               name of server for connection
   // COMMAND              command to execute
   // ENVIRONMENT          environment for command to execute
   // RESPONSE_TYPE        output type of the command (yes = response for client, no = request to server)
   // FOLLOW_REDIRECTS     if yes manage to automatically follow redirects from server
   // USER                 if     manage to follow redirects, in response to a HTTP_UNAUTHORISED response from the HTTP server: user
   // PASSWORD             if     manage to follow redirects, in response to a HTTP_UNAUTHORISED response from the HTTP server: password
   // -----------------------------------------------------------------------------------------------------------------------------------

   UString uri;
   UModProxyService* service;

   if (vmsg_error) (void) cfg.loadVector(*vmsg_error, "ERROR MESSAGE");

   while (cfg.searchForObjectStream())
      {
      service = U_NEW(UModProxyService);

      (void) cfg.loadVector(service->vreplace_response, "REPLACE_RESPONSE");

      if (cfg.loadTable())
         {
         uri                       = cfg[*UServer_Base::str_URI];
         service->host_mask        = cfg[*UServer_Base::str_HOST];
         service->method_mask      = cfg[*UServer_Base::str_METHOD_NAME];
         service->server           = cfg[*UServer_Base::str_SERVER];
         service->user             = cfg[*UServer_Base::str_USER];
         service->password         = cfg[*UServer_Base::str_PASSWORD];

         service->command          = UServer_Base::loadConfigCommand(cfg);
         service->environment      = service->command->getStringEnvironment();

         service->port             = cfg.readLong(*UServer_Base::str_PORT, 80);
         service->request_cert     = cfg.readBoolean(*str_CLIENT_CERTIFICATE);
         service->response_client  = cfg.readBoolean(*UServer_Base::str_RESPONSE_TYPE);
         service->follow_redirects = cfg.readBoolean(*str_FOLLOW_REDIRECTS);

         if (uri.empty() == false) service->uri_mask.set(uri, 0);

         vservice.push_back(service);

         cfg.clear();
         }
      }
}

UModProxyService* UModProxyService::findService(const UString& host, const UString& method, UVector<UModProxyService*>& vservice)
{
   U_TRACE(0, "UModProxyService::findService(%.*S,%.*S,%p)", U_STRING_TO_TRACE(host), U_STRING_TO_TRACE(method), &vservice)

   U_INTERNAL_DUMP("method = %.*S uri = %.*S", U_HTTP_METHOD_TO_TRACE, U_HTTP_URI_TO_TRACE)

   UModProxyService* elem;

   for (uint32_t i = 0, n = vservice.size(); i < n; ++i)
      {
      elem = vservice[i];

      U_INTERNAL_DUMP("method_mask = %.*S uri_mask = %.*S", U_STRING_TO_TRACE(elem->method_mask), U_STRING_TO_TRACE(elem->uri_mask.getMask()))

      if ((elem->host_mask.empty()       || UServices::DosMatchWithOR(host, elem->host_mask, 0))     &&
          (elem->method_mask.empty()     || UServices::DosMatchWithOR(method, elem->method_mask, 0)) &&
          (elem->uri_mask.getPcre() == 0 || elem->uri_mask.search(U_HTTP_URI_TO_PARAM)))
         {
         U_RETURN_POINTER(elem, UModProxyService);
         }
      }

   U_RETURN_POINTER(0, UModProxyService);
}

UString UModProxyService::replaceResponse(const UString& msg)
{
   U_TRACE(0, "UModProxyService::replaceResponse(%.*S)", U_STRING_TO_TRACE(msg))

   UString result = msg;

   for (int32_t i = 0, n = vreplace_response.size(); i < n; i += 2)
      {
      // Searches subject for matches to pattern and replaces them with replacement

      result = UStringExt::pregReplace(vreplace_response[i], vreplace_response[i+1], result);
      }

   U_RETURN_STRING(result);
}

void UModProxyService::setMsgError(int err, UVector<UString>& vmsg_error)
{
   U_TRACE(0, "UModProxyService::setMsgError(%d,%p)", err, &vmsg_error)

   /*
   INTERNAL_ERROR = 1, // NB: we need to start from 1 because we use a vector...
   BAD_REQUEST    = 2,
   NOT_FOUND      = 3,
   FORBIDDEN      = 4,
   */

   U_INTERNAL_ASSERT_RANGE(1,err,UModProxyService::ERROR_A_X509_NOBASICAUTH)

   switch (err)
      {
      case UModProxyService::INTERNAL_ERROR:   UHTTP::setHTTPInternalError(); break;
      case UModProxyService::BAD_REQUEST:      UHTTP::setHTTPBadRequest();    break;
      case UModProxyService::NOT_FOUND:        UHTTP::setHTTPNotFound();      break;
      case UModProxyService::FORBIDDEN:        UHTTP::setHTTPForbidden();     break;

      default:
         {
         if (vmsg_error.empty() == false)
            {
            UString fmt = vmsg_error[err - UModProxyService::FORBIDDEN - 1];

            UClientImage_Base::wbuffer->setBuffer(100U + fmt.size());
            UClientImage_Base::wbuffer->snprintf(fmt.c_str());
            }
         }
      }
}

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UModProxyService::dump(bool reset) const
{
   *UObjectIO::os << "port                                " << port                       << '\n'
                  << "request_cert                        " << request_cert               << '\n'
                  << "response_client                     " << response_client            << '\n'
                  << "follow_redirects                    " << follow_redirects           << '\n'
                  << "uri_mask          (UPCRE            " << (void*)&uri_mask           << ")\n"
                  << "method_mask       (UPCRE            " << (void*)&method_mask        << ")\n"
                  << "user              (UString          " << (void*)&user               << ")\n"
                  << "server            (UString          " << (void*)&server             << ")\n"
                  << "host_mask         (UString          " << (void*)&host_mask          << ")\n"
                  << "password          (UString          " << (void*)&password           << ")\n"
                  << "environment       (UString          " << (void*)&environment        << ")\n"
                  << "vreplace_response (UVector<UString> " << (void*)&vreplace_response  << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}
#endif

#endif
