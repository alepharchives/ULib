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
#include <ulib/net/client/client.h>
#include <ulib/net/server/server.h>
#include <ulib/net/server/plugin/mod_scgi.h>

#ifndef __MINGW32__
#  include <ulib/net/unixsocket.h>
#endif

#ifdef HAVE_MODULES
U_CREAT_FUNC(mod_scgi, USCGIPlugIn)
#endif

bool           USCGIPlugIn::scgi_keep_conn;
UClient_Base*  USCGIPlugIn::connection;

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

USCGIPlugIn::USCGIPlugIn()
{
   U_TRACE_REGISTER_OBJECT_WITHOUT_CHECK_MEMORY(0, USCGIPlugIn, "")

   if (str_SCGI_URI_MASK == 0) str_allocate();
}

USCGIPlugIn::~USCGIPlugIn()
{
   U_TRACE_UNREGISTER_OBJECT(0, USCGIPlugIn)

   if (connection)           delete connection;
   if (UHTTP::scgi_uri_mask) delete UHTTP::scgi_uri_mask;
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
      scgi_keep_conn = cfg.readBoolean(*str_SCGI_KEEP_CONN);

      connection = U_NEW(UClient_Base(&cfg));

      UString x = cfg[*str_SCGI_URI_MASK];

      U_INTERNAL_ASSERT_EQUALS(UHTTP::scgi_uri_mask,0)

      if (x.empty() == false) UHTTP::scgi_uri_mask = U_NEW(UString(x));
      }

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

int USCGIPlugIn::handlerInit()
{
   U_TRACE(1, "USCGIPlugIn::handlerInit()")

   if (connection &&
       UHTTP::scgi_uri_mask)
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

         (void) UServer_Base::senvironment->append(U_CONSTANT_TO_PARAM("SCGI=1\n"));

         // NB: SCGI is NOT a static page...

         if (UHTTP::valias == 0) UHTTP::valias = U_NEW(UVector<UString>(2U));

         UHTTP::valias->push_back(*UHTTP::scgi_uri_mask);
         UHTTP::valias->push_back(U_STRING_FROM_CONSTANT("/nostat"));

         U_RETURN(U_PLUGIN_HANDLER_GO_ON);
         }

      delete connection;
             connection = 0;
      }

   U_SRV_LOG("initialization of plugin FAILED");

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

// Connection-wide hooks

int USCGIPlugIn::handlerRequest()
{
   U_TRACE(0, "USCGIPlugIn::handlerRequest()")

   U_INTERNAL_ASSERT_POINTER(connection)
   U_INTERNAL_ASSERT_POINTER(UHTTP::scgi_uri_mask)

   if (u_dosmatch_with_OR(U_HTTP_URI_TO_PARAM, U_STRING_TO_PARAM(*UHTTP::scgi_uri_mask), 0))
      {
      // Set environment for the SCGI application server

      char* equalPtr;
      char* envp[128];
      UString environment = UHTTP::getCGIEnvironment(true);

      if (environment.empty())
         {
         UHTTP::setBadRequest();

         U_RETURN(U_PLUGIN_HANDLER_ERROR);
         }

      int n = u_split(U_STRING_TO_PARAM(environment), envp, 0);

      U_INTERNAL_ASSERT_MINOR(n, 128)

#  if defined(DEBUG) || defined(U_TEST)
      uint32_t hlength = 0; // calculate the total length of the headers
#  endif

      for (int i = 0; i < n; ++i)
         {
         equalPtr = strchr(envp[i], '=');

         U_INTERNAL_ASSERT_POINTER(equalPtr)
         U_INTERNAL_ASSERT_MAJOR(equalPtr-envp[i], 0)
         U_INTERNAL_ASSERT_MAJOR(u__strlen(equalPtr+1, __PRETTY_FUNCTION__), 0)

#     if defined(DEBUG) || defined(U_TEST)
         hlength += (equalPtr - envp[i]) + u__strlen(equalPtr, __PRETTY_FUNCTION__) + 1;
#     endif

         *equalPtr = '\0';
         }

      n = environment.size();

      U_INTERNAL_ASSERT_EQUALS((int)hlength, n)

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

err:  UHTTP::setInternalError();
end:  UHTTP::setRequestProcessed();

      // reset

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
                  << "connection    (UClient_Base " << (void*)connection      << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
