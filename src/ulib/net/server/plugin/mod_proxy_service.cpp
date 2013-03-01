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
#include <ulib/net/server/plugin/mod_proxy_service.h>

#ifdef HAVE_STRSTREAM_H
#  include <strstream.h>
#else
#  include <ulib/replace/strstream.h>
#endif

const UString* UModProxyService::str_FOLLOW_REDIRECTS;
const UString* UModProxyService::str_CLIENT_CERTIFICATE;
const UString* UModProxyService::str_REMOTE_ADDRESS_IP;
const UString* UModProxyService::str_WEBSOCKET;

void UModProxyService::str_allocate()
{
   U_TRACE(0, "UModProxyService::str_allocate()")

   U_INTERNAL_ASSERT_EQUALS(str_FOLLOW_REDIRECTS,0)
   U_INTERNAL_ASSERT_EQUALS(str_CLIENT_CERTIFICATE,0)
   U_INTERNAL_ASSERT_EQUALS(str_REMOTE_ADDRESS_IP,0)

   static ustringrep stringrep_storage[] = {
      { U_STRINGREP_FROM_CONSTANT("FOLLOW_REDIRECTS") },
      { U_STRINGREP_FROM_CONSTANT("CLIENT_CERTIFICATE") },
      { U_STRINGREP_FROM_CONSTANT("REMOTE_ADDRESS_IP") },
      { U_STRINGREP_FROM_CONSTANT("WEBSOCKET") }
   };

   U_NEW_ULIB_OBJECT(str_FOLLOW_REDIRECTS,   U_STRING_FROM_STRINGREP_STORAGE(0));
   U_NEW_ULIB_OBJECT(str_CLIENT_CERTIFICATE, U_STRING_FROM_STRINGREP_STORAGE(1));
   U_NEW_ULIB_OBJECT(str_REMOTE_ADDRESS_IP,  U_STRING_FROM_STRINGREP_STORAGE(2));
   U_NEW_ULIB_OBJECT(str_WEBSOCKET,          U_STRING_FROM_STRINGREP_STORAGE(3));
}

UModProxyService::UModProxyService()
{
   U_TRACE_REGISTER_OBJECT(0, UModProxyService, "")

   vremote_address = 0;

   if (str_FOLLOW_REDIRECTS == 0) str_allocate();
}

UModProxyService::~UModProxyService()
{
   U_TRACE_UNREGISTER_OBJECT(0, UModProxyService)

   if (vremote_address) delete vremote_address;
}

void UModProxyService::loadConfig(UFileConfig& cfg, UVector<UModProxyService*>& vservice, UVector<UString>* vmsg_error)
{
   U_TRACE(0, "UModProxyService::loadConfig(%p,%p,%p)", &cfg, &vservice, vmsg_error)

   // -----------------------------------------------------------------------------------------------------------------------------------
   // mod_proxy - plugin parameters
   // -----------------------------------------------------------------------------------------------------------------------------------
   // ERROR MESSAGE        Allows you to tell clients about what type of error
   //
   // URI                  uri mask trigger
   // HOST                 name host client
   // METHOD_NAME          mask type of what type of HTTP method is considered (GET|POST)
   // CLIENT_CERTIFICATE   yes if client must comunicate a certificate in the SSL connection
   // REMOTE_ADDRESS_IP    list of comma separated client address for IP-based control (IPADDR[/MASK]) for routing-like policy
   // WEBSOCKET            yes if the proxy act as a Reverse Proxy Web Sockets
   //
   // COMMAND              command to execute
   // ENVIRONMENT          environment for command to execute
   // RESPONSE_TYPE        output type of the command (yes = response for client, no = request to server)
   //
   // PORT                 port of server for connection
   // SERVER               name of server for connection
   //
   // FOLLOW_REDIRECTS     yes if     manage to automatically follow redirects from server
   // USER                     if     manage to follow redirects, in response to a HTTP_UNAUTHORISED response from the HTTP server: user
   // PASSWORD                 if     manage to follow redirects, in response to a HTTP_UNAUTHORISED response from the HTTP server: password
   // REPLACE_RESPONSE         if NOT manage to follow redirects, maybe vector of substitution string
   // -----------------------------------------------------------------------------------------------------------------------------------

   UString x;
   UModProxyService* service;

   if (vmsg_error) (void) cfg.loadVector(*vmsg_error, "ERROR MESSAGE");

   while (cfg.searchForObjectStream())
      {
      service = U_NEW(UModProxyService);

      (void) cfg.loadVector(service->vreplace_response, "REPLACE_RESPONSE");

      if (cfg.loadTable())
         {
         x                         = cfg[*UServer_Base::str_URI];
         service->host_mask        = cfg[*UServer_Base::str_HOST];
         service->method_mask      = cfg[*UServer_Base::str_METHOD_NAME];
         service->server           = cfg[*UServer_Base::str_SERVER];
         service->user             = cfg[*UServer_Base::str_USER];
         service->password         = cfg[*UServer_Base::str_PASSWORD];

         service->port             = cfg.readLong(*UServer_Base::str_PORT, 80);
         service->websocket        = cfg.readBoolean(*str_WEBSOCKET);
         service->request_cert     = cfg.readBoolean(*str_CLIENT_CERTIFICATE);
         service->response_client  = cfg.readBoolean(*UServer_Base::str_RESPONSE_TYPE);
         service->follow_redirects = cfg.readBoolean(*str_FOLLOW_REDIRECTS);

         if (x.empty() == false) service->uri_mask.set(x, 0);

         if ((service->command = UServer_Base::loadConfigCommand(cfg))) service->environment = service->command->getStringEnvironment();

         // REMOTE ADDRESS IP

         x = cfg[*str_REMOTE_ADDRESS_IP];

         if (x.empty() == false)
            {
            service->vremote_address = U_NEW(UVector<UIPAllow*>);

            if (UIPAllow::parseMask(x, *(service->vremote_address)) == 0)
               {
               delete service->vremote_address;
                      service->vremote_address = 0;
               }
            }

         vservice.push_back(service);

         cfg.clear();
         }
      }
}

UModProxyService*
UModProxyService::findService(const char* host, uint32_t host_len, const char* method, uint32_t method_len, UVector<UModProxyService*>& vservice)
{
   U_TRACE(0, "UModProxyService::findService(%.*S,%u,%.*S,%u,%p)", host_len, host, host_len, method_len, method, method_len, &vservice)

   U_INTERNAL_DUMP("uri = %.*S", U_HTTP_URI_TO_TRACE)

   UModProxyService* elem;
   UString uri = UHTTP::getRequestURI();

   for (uint32_t i = 0, n = vservice.size(); i < n; ++i)
      {
      elem = vservice[i];

      U_INTERNAL_DUMP("host_mask = %.*S method_mask = %.*S uri_mask = %.*S", U_STRING_TO_TRACE(elem->host_mask),
                                                                             U_STRING_TO_TRACE(elem->method_mask),
                                                                             U_STRING_TO_TRACE(elem->uri_mask.getMask()))

      if ((elem->vremote_address == 0    || UServer_Base::pClientImage->isAllowed(*(elem->vremote_address)))                 &&
          (elem->host_mask.empty()       || u_dosmatch_with_OR(host,     host_len, U_STRING_TO_PARAM(elem->host_mask),   0)) &&
          (elem->method_mask.empty()     || u_dosmatch_with_OR(method, method_len, U_STRING_TO_PARAM(elem->method_mask), 0)) &&
          (elem->uri_mask.getPcre() == 0 || elem->uri_mask.search(U_STRING_TO_PARAM(uri))))
         {
         U_RETURN_POINTER(elem, UModProxyService);
         }
      }

   U_RETURN_POINTER(0, UModProxyService);
}

UString UModProxyService::getServer() const
{
   U_TRACE(0, "UModProxyService::getServer()")

   char c = server.c_char(0);

   if (c == '~' ||
       c == '$')
      {
      UString x;

      if (server.c_char(1) == '<')
         {
         (void) x.assign(U_HTTP_VHOST_TO_PARAM);

         U_RETURN_STRING(x);
         }

      x = UStringExt::expandPath(server, 0);

      if (x.empty() == false)
         {
         UString pathname(U_CAPACITY);

         pathname.snprintf("%.*s/%s:%u.srv", U_STRING_TO_TRACE(x), UServer_Base::client_address, UHTTP::getUserAgent());

         x = UFile::contentOf(pathname);

         if (x.empty() == false)
            {
            U_INTERNAL_ASSERT_EQUALS(x.someWhiteSpace(),false)

            U_RETURN_STRING(x);
            }
         }
      }

   U_RETURN_STRING(server);
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
      case UModProxyService::INTERNAL_ERROR:   UHTTP::setInternalError(); break;
      case UModProxyService::BAD_REQUEST:      UHTTP::setBadRequest();    break;
      case UModProxyService::NOT_FOUND:        UHTTP::setNotFound();      break;
      case UModProxyService::FORBIDDEN:        UHTTP::setForbidden();     break;

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
   U_CHECK_MEMORY

   *UObjectIO::os << "port                                 " << port                       << '\n'
                  << "websocket                            " << websocket                  << '\n'
                  << "request_cert                         " << request_cert               << '\n'
                  << "response_client                      " << response_client            << '\n'
                  << "follow_redirects                     " << follow_redirects           << '\n'
                  << "uri_mask          (UPCRE             " << (void*)&uri_mask           << ")\n"
                  << "user              (UString           " << (void*)&user               << ")\n"
                  << "server            (UString           " << (void*)&server             << ")\n"
                  << "host_mask         (UString           " << (void*)&host_mask          << ")\n"
                  << "password          (UString           " << (void*)&password           << ")\n"
                  << "environment       (UString           " << (void*)&environment        << ")\n"
                  << "method_mask       (UString           " << (void*)&method_mask        << ")\n"
                  << "vremote_address   (UVector<UIPAllow> " << (void*)vremote_address     << ")\n"
                  << "vreplace_response (UVector<UString>  " << (void*)&vreplace_response  << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}
#endif

#endif
