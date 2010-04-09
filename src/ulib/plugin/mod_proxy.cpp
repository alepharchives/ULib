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
#include <ulib/plugin/mod_proxy.h>
#include <ulib/net/server/server.h>
#include <ulib/net/server/client_image.h>
#include <ulib/plugin/mod_proxy_service.h>

U_CREAT_FUNC(UProxyPlugIn)

// Server-wide hooks

int UProxyPlugIn::handlerConfig(UFileConfig& cfg)
{
   U_TRACE(0, "UProxyPlugIn::handlerConfig(%p)", &cfg)

   UModProxyService::loadConfig(cfg, vservice, &vmsg_error);

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

int UProxyPlugIn::handlerInit()
{
   U_TRACE(0, "UProxyPlugIn::handlerInit()")

   if (vservice.empty())
      {
      U_SRV_LOG_MSG("initialization of plugin FAILED");

      U_RETURN(U_PLUGIN_HANDLER_ERROR);
      }

   if (UServer_Base::preforked_num_kids == 0) client_http.setHostForbidden(UServer_Base::getHost());

   U_SRV_LOG_MSG("initialization of plugin success");

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

// Connection-wide hooks

int UProxyPlugIn::handlerRequest()
{
   U_TRACE(0, "UProxyPlugIn::handlerRequest()")

   U_INTERNAL_DUMP("method = %.*S uri = %.*S", U_HTTP_METHOD_TO_TRACE, U_HTTP_URI_TO_TRACE)

   // find proxy service for the HTTP request

   UString host(U_HTTP_HOST_TO_PARAM),
           method = UHTTP::getHTTPMethod();

   UModProxyService* service = UModProxyService::findService(host, method, vservice);

   if (service == 0) U_RETURN(U_PLUGIN_HANDLER_GO_ON);

   // process the HTTP request

   int err = 0;
   bool esito,
        output_to_client  = false, // send output as response to client...
        output_to_server  = false; // send output as request  to server...

   is_connect = false;

#ifdef HAVE_SSL
   // check if certificate is required...

   if (service->isRequestCertificate() &&
       ((UServer<USSLSocket>*)UServer_Base::pthis)->askForClientCertificate() == false)
      {
      err = UModProxyService::BAD_REQUEST;

      goto error;
      }
#endif

   // check if action required

   if (service->command)
      {
      UCommand* pcmd = service->command;

      UString     command = pcmd->getStringCommand(),
              environment = pcmd->getStringEnvironment();

      // NB: we need this because processCGIRequest() can split the string...

          command.duplicate();
      environment.duplicate();

      esito = UHTTP::processCGIRequest(pcmd, 0);

      pcmd->reset(command, environment);

      if (esito == false) goto end; // skip error...

      if (service->isResponseForClient()) output_to_client = true; // send output as response to client...
      else                                output_to_server = true; // send output as request  to server...
      }

   U_INTERNAL_DUMP("output_to_server = %b output_to_client = %b", output_to_server, output_to_client)

   if (output_to_server) // check if the request is HTTP...
      {
      U_ASSERT_EQUALS(UClientImage_Base::wbuffer->empty(), false)

      const char* ptr = UClientImage_Base::wbuffer->data();

      if (                        UHTTP::isHTTPRequest(  ptr)  == false ||
         (UHTTP::resetHTTPInfo(), UHTTP::scanfHTTPHeader(ptr)) == false)
         {
         err = UModProxyService::INTERNAL_ERROR;

         goto error;
         }

      U_INTERNAL_DUMP("method = %.*S uri = %.*S", U_HTTP_METHOD_TO_TRACE, U_HTTP_URI_TO_TRACE)
      }

   if (output_to_client == false)
      {
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

      client_http.setFollowRedirects(service->isFollowRedirects());

      // ...but before check if server and/or port to connect has changed...

      if (client_http.setHostPort(server, port)) client_http.UClient_Base::close();

      is_connect = client_http.isConnected();

      for (int counter = 0; counter < 2; ++counter)
         {
         if (is_connect ||
             client_http.UClient_Base::connect())
            {
            // send request to server and get response

            if (output_to_server == false) *UClientImage_Base::wbuffer = *UClientImage_Base::rbuffer;

            (void) client_http.sendRequest(*UClientImage_Base::wbuffer);

            // reset reference to rbuffer...

            *UClientImage_Base::wbuffer = client_http.getResponse();

            // check if server have not closed the connection (timeout of Keep-Alive, ...)

            is_connect = client_http.isConnected();

            if (is_connect) break;
            }
         }

      client_http.UClient_Base::reset();

      if (is_connect == false)
         {
         U_SRV_LOG_MSG(client_http.getResponseData());

         err = UModProxyService::INTERNAL_ERROR;

         goto error;
         }

      // check if request replace response from server for this service...

      if (service->isReplaceResponse()) *UClientImage_Base::wbuffer = service->replaceResponse(*UClientImage_Base::wbuffer); 

      if (UClientImage_Base::wbuffer->empty())
         {
         err = UModProxyService::INTERNAL_ERROR;

         goto error;
         }
      }

   goto end; // skip error...

error:
   UModProxyService::setMsgError(err, vmsg_error);

end:
   U_RETURN(U_PLUGIN_HANDLER_FINISHED);
}

int UProxyPlugIn::handlerReset()
{
   U_TRACE(0, "UProxyPlugIn::handlerReset()")

   if (is_connect) UClientImage_Base::wbuffer->clear(); // NB: reset reference to client_http.response...

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UProxyPlugIn::dump(bool reset) const
{
   *UObjectIO::os << "is_connect                              " << is_connect          << ")\n"
                  << "vmsg_error  (UVector<UString>           " << (void*)&vmsg_error  << ")\n"
                  << "client_http (UHttpClient<USocket>       " << (void*)&client_http << ")\n"
                  << "vservice    (UVector<UModProxyService*> " << (void*)&vservice    << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
