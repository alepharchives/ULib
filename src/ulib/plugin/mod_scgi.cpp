// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    mod_scgi.cpp - Perform simple scgi request forwarding
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/file_config.h>
#include <ulib/utility/uhttp.h>
#include <ulib/net/tcpsocket.h>
#include <ulib/plugin/mod_scgi.h>
#include <ulib/net/client/client.h>
#include <ulib/net/server/server.h>

#ifndef __MINGW32__
#  include <ulib/net/unixsocket.h>
#endif

U_CREAT_FUNC(mod_scgi, USCGIPlugIn)

const UString* USCGIPlugIn::str_SCGI_URI_MASK;
const UString* USCGIPlugIn::str_SCGI_KEEP_CONN;

void USCGIPlugIn::str_allocate()
{
   U_TRACE(0, "USCGIPlugIn::str_allocate()")

   U_INTERNAL_ASSERT_EQUALS(str_SCGI_URI_MASK,0)
   U_INTERNAL_ASSERT_EQUALS(str_SCGI_KEEP_CONN,0)

   static ustringrep stringrep_storage[] = {
      { U_STRINGREP_FROM_CONSTANT("SCGI_URI_MASK") },
      { U_STRINGREP_FROM_CONSTANT("SCGI_KEEP_CONN") }
   };

   U_NEW_ULIB_OBJECT(str_SCGI_URI_MASK,  U_STRING_FROM_STRINGREP_STORAGE(0));
   U_NEW_ULIB_OBJECT(str_SCGI_KEEP_CONN, U_STRING_FROM_STRINGREP_STORAGE(1));
}

USCGIPlugIn::~USCGIPlugIn()
{
   U_TRACE_UNREGISTER_OBJECT(0, USCGIPlugIn)

   if (connection) delete connection;
}

// Server-wide hooks

int USCGIPlugIn::handlerConfig(UFileConfig& cfg)
{
   U_TRACE(0, "USCGIPlugIn::handlerConfig(%p)", &cfg)

   // ------------------------------------------------------------------------------------------
   // SCGI_URI_MASK  mask (DOS regexp) of uri type that send request to SCGI (*.php)
   //
   // NAME_SOCKET    file name for the scgi socket
   //
   // USE_IPV6       flag to indicate use of ipv6
   // SERVER         host name or ip address for the scgi host
   // PORT           port number             for the scgi host
   //
   // RES_TIMEOUT    timeout for response from server SCGI
   // SCGI_KEEP_CONN If not zero, the server SCGI does not close the connection after
   //                responding to request; the plugin retains responsibility for the connection.
   //
   // LOG_FILE       location for file log (use server log if exist)
   // ------------------------------------------------------------------------------------------

   if (cfg.loadTable())
      {
      scgi_uri_mask  = cfg[*str_SCGI_URI_MASK];
      scgi_keep_conn = cfg.readBoolean(*str_SCGI_KEEP_CONN);

      connection = U_NEW(UClient_Base(&cfg));
      }

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

int USCGIPlugIn::handlerInit()
{
   U_TRACE(1, "USCGIPlugIn::handlerInit()")

   if (connection &&
       scgi_uri_mask.empty() == false)
      {
#  ifdef __MINGW32__
      U_INTERNAL_ASSERT_DIFFERS(connection->port, 0)

      connection->socket = (USocket*)U_NEW(UTCPSocket(connection->bIPv6));
#  else
      connection->socket = connection->port ? (USocket*)U_NEW(UTCPSocket(connection->bIPv6))
                                            : (USocket*)U_NEW(UUnixSocket);
#  endif

      if (connection->connect())
         {
         U_SRV_LOG("connection to the scgi-backend %.*S accepted", U_STRING_TO_TRACE(connection->host_port));

         U_SRV_LOG("initialization of plugin success");

         goto end;
         }
      }

   U_SRV_LOG("initialization of plugin FAILED");

end:
   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

// Connection-wide hooks

int USCGIPlugIn::handlerRequest()
{
   U_TRACE(0, "USCGIPlugIn::handlerRequest()")

   if (UHTTP::isHTTPRequestAlreadyProcessed() ||
       scgi_uri_mask.empty()                  ||
       u_dosmatch_with_OR(U_HTTP_URI_TO_PARAM, U_STRING_TO_PARAM(scgi_uri_mask), 0) == false)
      {
      U_RETURN(U_PLUGIN_HANDLER_GO_ON);
      }

   if (connection &&
       connection->isConnected())
      {
      // Set environment for the SCGI application server

      char* equalPtr;
      char* envp[128];
      UString environment = UHTTP::getCGIEnvironment() + "SCGI=1\n" + *UHTTP::penvironment;
      int n = u_split(U_STRING_TO_PARAM(environment), envp, 0);

      U_INTERNAL_ASSERT_MINOR(n, 128)

#  if defined(DEBUG) || (defined(U_TEST) && !defined(__CYGWIN__) && !defined(__MINGW32__))
      uint32_t hlength = 0; // calculate the total length of the headers
#  endif

      for (int i = 0; i < n; ++i)
         {
         equalPtr = strchr(envp[i], '=');

         U_INTERNAL_ASSERT_POINTER(equalPtr)
         U_INTERNAL_ASSERT_MAJOR(equalPtr-envp[i], 0)
         U_INTERNAL_ASSERT_MAJOR(u_strlen(equalPtr+1), 0)

#     if defined(DEBUG) || (defined(U_TEST) && !defined(__CYGWIN__) && !defined(__MINGW32__))
         hlength += (equalPtr - envp[i]) + u_strlen(equalPtr) + 1;
#     endif

         *equalPtr = '\0';
         }

      n = environment.size();

      U_INTERNAL_ASSERT_EQUALS(hlength, n)

      // send header data as netstring -> [len]":"[string]","

      UString request(10U + n);

      request.snprintf("%u:%.*s,", environment.size(), U_STRING_TO_TRACE(environment));

      (void) request.append(*UClientImage_Base::body);

      if (connection->sendRequest(request, true) == false) goto err;

      if (scgi_keep_conn == false)
         {
         /* The shutdown() tells the receiver the server is done sending data. No
          * more data is going to be send. More importantly, it doesn't close the
          * socket. At the socket layer, this sends a TCP/IP FIN packet to the receiver
          */

         if (connection->shutdown() == false) goto err;
         }

      *UClientImage_Base::wbuffer = connection->getResponse();

      (void) UHTTP::processCGIOutput();

      goto end; // skip error...

err:  UHTTP::setHTTPInternalError();
end:  UHTTP::setHTTPRequestProcessed();
      }

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

int USCGIPlugIn::handlerReset()
{
   U_TRACE(0, "USCGIPlugIn::handlerReset()")

   if (connection)
      {
      connection->clearData();

      if (scgi_keep_conn == false &&
          connection->isConnected())
         {
         connection->close();
         }
      }

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* USCGIPlugIn::dump(bool reset) const
{
   *UObjectIO::os << "scgi_keep_conn              " << scgi_keep_conn         <<  '\n'
                  << "scgi_uri_mask (UString      " << (void*)&scgi_uri_mask  << ")\n"
                  << "connection    (UClient_Base " << (void*)connection      << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
