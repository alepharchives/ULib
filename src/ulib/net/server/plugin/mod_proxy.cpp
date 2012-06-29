// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    mod_proxy.cpp - this is a plugin proxy for userver
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/command.h>
#include <ulib/utility/escape.h>
#include <ulib/net/server/server.h>
#include <ulib/net/server/client_image.h>
#include <ulib/net/server/plugin/mod_proxy.h>
#include <ulib/net/server/plugin/mod_proxy_service.h>

#ifdef HAVE_MODULES
U_CREAT_FUNC(mod_proxy, UProxyPlugIn)
#endif

UProxyPlugIn::~UProxyPlugIn()
{
   U_TRACE_UNREGISTER_OBJECT(0, UProxyPlugIn)

   if (vservice) delete vservice;
}

// Server-wide hooks

int UProxyPlugIn::handlerConfig(UFileConfig& cfg)
{
   U_TRACE(0, "UProxyPlugIn::handlerConfig(%p)", &cfg)

   vservice = U_NEW(UVector<UModProxyService*>);

   UModProxyService::loadConfig(cfg, *vservice, &vmsg_error);

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

int UProxyPlugIn::handlerInit()
{
   U_TRACE(0, "UProxyPlugIn::handlerInit()")

   if (vservice == 0 ||
       vservice->empty())
      {
      U_SRV_LOG("initialization of plugin FAILED");

      goto end;
      }

   if (UServer_Base::preforked_num_kids == 0) client_http.setLocalHost(UServer_Base::getHost());

   U_SRV_LOG("initialization of plugin success");

end:
   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

// Connection-wide hooks

int UProxyPlugIn::handlerRequest()
{
   U_TRACE(0, "UProxyPlugIn::handlerRequest()")

   // ------------------------------------------------------------------------------
   // find the eventaully proxy service for the HTTP request
   // ------------------------------------------------------------------------------
   // The difference between U_HTTP_HOST_.. and U_HTTP_VHOST_.. is that
   // U_HTTP_HOST_.. can include the «:PORT» text, and U_HTTP_VHOST_.. only the name
   // ------------------------------------------------------------------------------

   UModProxyService* service = (vservice ? UModProxyService::findService(UString(U_HTTP_VHOST_TO_PARAM), UHTTP::getHTTPMethod(), *vservice) : 0);

   if (service == 0) U_RETURN(U_PLUGIN_HANDLER_GO_ON);

   // process the HTTP request

   int err = 0;
   bool output_to_client = false, // send output as response to client...
        output_to_server = false; // send output as request  to server...

   // NB: if server no preforked (ex: nodog) process the HTTP request with fork....

   bool async = (UServer_Base::preforked_num_kids == 0 &&
                 UClientImage_Base::isPipeline()  == false);

#ifdef USE_LIBSSL // check if certificate is required...
   if (service->isRequestCertificate() &&
       ((UServer<USSLSocket>*)UServer_Base::pthis)->askForClientCertificate() == false)
      {
      err = UModProxyService::BAD_REQUEST;

      goto err;
      }
#endif

   // check if action required

   if (service->command)
      {
      if (UHTTP::processCGIRequest(service->command, &(service->environment), async, true) == false) goto end;

      if (service->isResponseForClient()) output_to_client = true; // send output as response to client...
      else                                output_to_server = true; // send output as request  to server...
      }

   U_INTERNAL_DUMP("output_to_server = %b output_to_client = %b", output_to_server, output_to_client)

   if (output_to_server) // check if the request is HTTP...
      {
      const char* ptr = UClientImage_Base::wbuffer->data();

      U_ASSERT_EQUALS(UClientImage_Base::wbuffer->empty(), false)

      if (                     UHTTP::isHTTPRequest(  ptr)  == false ||
         (U_HTTP_INFO_INIT(0), UHTTP::scanfHTTPHeader(ptr)) == false)
         {
         err = UModProxyService::INTERNAL_ERROR;

         goto err;
         }

      U_INTERNAL_DUMP("method = %.*S uri = %.*S", U_HTTP_METHOD_TO_TRACE, U_HTTP_URI_TO_TRACE)
      }

   if (output_to_client == false)
      {
      bool result;

      // connect to server...

      int port       = service->getPort();
      UString user   = service->getUser(),
              server = service->getServer(),
              passwd = service->getPassword();

      if (  user.empty() == false &&
          passwd.empty() == false)
         {
         client_http.setRequestPasswordAuthentication(user, passwd);
         }

      client_http.setFollowRedirects(service->isFollowRedirects(), true);

      if (async &&
          UServer_Base::parallelization())
         {
         client_http.UClient_Base::reset();

         goto end; // skip error...
         }

      // ...but before check if server and/or port to connect has changed...

      if (client_http.setHostPort(server, port) &&
          client_http.UClient_Base::isConnected())
         {
         client_http.UClient_Base::close();
         }

      // send request to server and get response

      if (output_to_server == false) *UClientImage_Base::wbuffer = *UClientImage_Base::request;

      result = client_http.sendRequest(*UClientImage_Base::wbuffer);

      if (result)
         {
                                           *UClientImage_Base::wbuffer = client_http.getResponse();
         if (service->isReplaceResponse()) *UClientImage_Base::wbuffer = service->replaceResponse(*UClientImage_Base::wbuffer); 
         }
      else if (UServer_Base::isLog())
         {
         UString resp = client_http.getResponse();
         uint32_t sz  = resp.size();

         if (sz)
            {
            const char* ptr = resp.data();
            uint32_t u_printf_string_max_length_save = u_printf_string_max_length;

            U_INTERNAL_DUMP("u_printf_string_max_length = %d", u_printf_string_max_length)

            u_printf_string_max_length = u_findEndHeader(ptr, sz);

            if (u_printf_string_max_length == -1) u_printf_string_max_length = sz;

            U_INTERNAL_ASSERT_MAJOR(u_printf_string_max_length, 0)

            U_INTERNAL_DUMP("u_printf_string_max_length = %d", u_printf_string_max_length)

            UServer_Base::log->log("%.*sproxy-server response (%u bytes) %.*S\n", U_STRING_TO_TRACE(*UServer_Base::mod_name), sz, sz, ptr);

            u_printf_string_max_length = u_printf_string_max_length_save;
            }
         }

      client_http.UClient_Base::reset(); // reset reference to request...
      }

   goto end;

err: UModProxyService::setMsgError(err, vmsg_error);

end: UHTTP::setHTTPRequestProcessed();

   // check for "Connection: close" in headers

   U_INTERNAL_DUMP("U_http_is_connection_close = %d", U_http_is_connection_close)

   if (U_http_is_connection_close == U_YES) U_RETURN(U_PLUGIN_HANDLER_ERROR);

   U_RETURN(U_PLUGIN_HANDLER_FINISHED);
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UProxyPlugIn::dump(bool reset) const
{
   *UObjectIO::os << "vmsg_error  (UVector<UString>           " << (void*)&vmsg_error  << ")\n"
                  << "client_http (UHttpClient<UTCPSocket>    " << (void*)&client_http << ")\n"
                  << "vservice    (UVector<UModProxyService*> " << (void*)vservice     << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
