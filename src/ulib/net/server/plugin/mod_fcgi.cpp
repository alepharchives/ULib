// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    mod_fcgi.cpp - Perform simple fastcgi request forwarding
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
#include <ulib/net/server/plugin/mod_fcgi.h>

#ifndef __MINGW32__
#  include <ulib/net/unixsocket.h>
#endif

/* spawn-fcgi is used to spawn remote and local FastCGI processes.
 *
 * While it is obviously needed to spawn remote FastCGI backends (the web server
 * can only spawn local ones), it is recommended to spawn local backends with spawn-fcgi, too.
 *
 * Reasons why you may want to use spawn-fcgi instead of something else:
 *
 * Privilege separation without needing a suid-binary or running a server as root.
 *
 * You can restart your web server and the FastCGI applications without restarting the others.
 *
 * You can run them in different chroot()s.
 *
 * Running your FastCGI applications doesn't depend on the web server you are running,
 * which allows for easier testing of other web servers.
 */

// ---------------------------------------------------------------------------------------------------------------
// START Fast CGI stuff
// ---------------------------------------------------------------------------------------------------------------
/*
 * Number of bytes in a FCGI_Header. Future versions of the protocol
 * will not reduce this number.
 */
#define FCGI_HEADER_LEN 8

/*
 * Value for version component of FCGI_Header
 */
#define FCGI_VERSION_1 1

/*
 * Values for type component of FCGI_Header
 */
#define FCGI_BEGIN_REQUEST       1
#define FCGI_ABORT_REQUEST       2
#define FCGI_END_REQUEST         3
#define FCGI_PARAMS              4
#define FCGI_STDIN               5
#define FCGI_STDOUT              6
#define FCGI_STDERR              7
#define FCGI_DATA                8
#define FCGI_GET_VALUES          9
#define FCGI_GET_VALUES_RESULT  10
#define FCGI_UNKNOWN_TYPE       11
#define FCGI_MAXTYPE (FCGI_UNKNOWN_TYPE)

/*
 * Value for requestId component of FCGI_Header
 */
#define FCGI_NULL_REQUEST_ID 0

typedef struct {
   u_char  version;
   u_char  type;
   u_short request_id;
   u_short content_length;
   u_char  padding_length;
   u_char  reserved;
} FCGI_Header;

/*
 * Mask for flags component of FCGI_BeginRequestBody
 */
#define FCGI_KEEP_CONN 1

/*
 * Values for role component of FCGI_BeginRequestBody
 */
#define FCGI_RESPONDER  1
#define FCGI_AUTHORIZER 2
#define FCGI_FILTER     3

typedef struct {
   u_short role;
   u_char  flags;
   u_char  reserved[5];
} FCGI_BeginRequestBody;

/*
 * Values for protocolStatus component of FCGI_EndRequestBody
 */
#define FCGI_REQUEST_COMPLETE 0
#define FCGI_CANT_MPX_CONN    1
#define FCGI_OVERLOADED       2
#define FCGI_UNKNOWN_ROLE     3

typedef struct {
   u_int  app_status;
   u_char protocol_status;
   u_char reserved[3];
} FCGI_EndRequestBody;

typedef struct {
   FCGI_Header           header;
   FCGI_BeginRequestBody body;
} FCGI_BeginRequestRecord;

typedef struct {
   FCGI_Header         header;
   FCGI_EndRequestBody body;
} FCGI_EndRequestRecord;

// Initialize a fast CGI header record

static FCGI_BeginRequestRecord beginRecord;

void UFCGIPlugIn::set_FCGIBeginRequest()
{
   U_TRACE(0, "UFCGIPlugIn::set_FCGIBeginRequest()")

   beginRecord.header.version    = FCGI_VERSION_1;
   beginRecord.header.request_id = htons((uint16_t)u_pid); // NB: we aren't supporting multiplexing than we use always the same request-id...

   beginRecord.body.role         = htons(FCGI_RESPONDER);
   beginRecord.body.flags        = fcgi_keep_conn;
}

void UFCGIPlugIn::fill_FCGIBeginRequest(u_char type, u_short content_length)
{
   U_TRACE(0, "UFCGIPlugIn::fill_FCGIBeginRequest(%C,%d)", type, content_length)

// beginRecord.header.version        = FCGI_VERSION_1;
   beginRecord.header.type           = type;
// beginRecord.header.request_id     = htons((uint16_t)u_pid);
   beginRecord.header.content_length = htons(content_length);
// beginRecord.header.padding_length = padding_length;
// beginRecord.header.reserved       = 0;
}

// ---------------------------------------------------------------------------------------------------------------
// END Fast CGI stuff
// ---------------------------------------------------------------------------------------------------------------

#ifdef HAVE_MODULES
U_CREAT_FUNC(mod_fcgi, UFCGIPlugIn)
#endif

bool           UFCGIPlugIn::fcgi_keep_conn;
UClient_Base*  UFCGIPlugIn::connection;

const UString* UFCGIPlugIn::str_FCGI_URI_MASK;
const UString* UFCGIPlugIn::str_FCGI_KEEP_CONN;

void UFCGIPlugIn::str_allocate()
{
   U_TRACE(0, "UFCGIPlugIn::str_allocate()")

   U_INTERNAL_ASSERT_EQUALS(str_FCGI_URI_MASK,0)
   U_INTERNAL_ASSERT_EQUALS(str_FCGI_KEEP_CONN,0)

   static ustringrep stringrep_storage[] = {
      { U_STRINGREP_FROM_CONSTANT("FCGI_URI_MASK") },
      { U_STRINGREP_FROM_CONSTANT("FCGI_KEEP_CONN") }
   };

   U_NEW_ULIB_OBJECT(str_FCGI_URI_MASK,  U_STRING_FROM_STRINGREP_STORAGE(0));
   U_NEW_ULIB_OBJECT(str_FCGI_KEEP_CONN, U_STRING_FROM_STRINGREP_STORAGE(1));
}

UFCGIPlugIn::UFCGIPlugIn()
{
   U_TRACE_REGISTER_OBJECT(0, UFCGIPlugIn, "")

   if (str_FCGI_URI_MASK == 0) str_allocate();
}

UFCGIPlugIn::~UFCGIPlugIn()
{
   U_TRACE_UNREGISTER_OBJECT(0, UFCGIPlugIn)

   if (connection)           delete connection;
   if (UHTTP::fcgi_uri_mask) delete UHTTP::fcgi_uri_mask;
}

// Server-wide hooks

int UFCGIPlugIn::handlerConfig(UFileConfig& cfg)
{
   U_TRACE(0, "UFCGIPlugIn::handlerConfig(%p)", &cfg)

   // ------------------------------------------------------------------------------------------
   // FCGI_URI_MASK  mask (DOS regexp) of uri type that send request to FCGI (*.php)
   //
   // NAME_SOCKET    file name for the fcgi socket
   //
   // SERVER         host name or ip address for the fcgi host
   // PORT           port number             for the fcgi host
   //
   // RES_TIMEOUT    timeout for response from server FCGI
   // FCGI_KEEP_CONN If not zero, the server FCGI does not close the connection after
   //                responding to request; the plugin retains responsibility for the connection.
   //
   // LOG_FILE       location for file log (use server log if exist)
   // ------------------------------------------------------------------------------------------

   if (cfg.loadTable())
      {
      fcgi_keep_conn = cfg.readBoolean(*str_FCGI_KEEP_CONN);

      connection = U_NEW(UClient_Base(&cfg));

      UString x = cfg[*str_FCGI_URI_MASK];

      U_INTERNAL_ASSERT_EQUALS(UHTTP::fcgi_uri_mask,0)

      if (x.empty() == false) UHTTP::fcgi_uri_mask = U_NEW(UString(x));
      }

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

int UFCGIPlugIn::handlerInit()
{
   U_TRACE(1, "UFCGIPlugIn::handlerInit()")

   if (connection &&
       UHTTP::fcgi_uri_mask)
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
         U_SRV_LOG("connection to the fastcgi-backend %.*S accepted", U_STRING_TO_TRACE(connection->host_port));

         U_SRV_LOG("initialization of plugin success");

         set_FCGIBeginRequest();

         // NB: FCGI is NOT a static page...

         if (UHTTP::valias == 0) UHTTP::valias = U_NEW(UVector<UString>(2U));

         UHTTP::valias->push_back(*UHTTP::fcgi_uri_mask);
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

int UFCGIPlugIn::handlerRequest()
{
   U_TRACE(0, "UFCGIPlugIn::handlerRequest()")

   U_INTERNAL_ASSERT_POINTER(connection)
   U_INTERNAL_ASSERT_POINTER(UHTTP::fcgi_uri_mask)

   if (u_dosmatch_with_OR(U_HTTP_URI_TO_PARAM, U_STRING_TO_PARAM(*UHTTP::fcgi_uri_mask), 0))
      {
      fill_FCGIBeginRequest(FCGI_BEGIN_REQUEST, sizeof(FCGI_BeginRequestBody));

      FCGI_Header* h;
      char* equalPtr;
      char* envp[128];
      uint32_t clength, pos, size;
      unsigned char  headerBuff[8];
      unsigned char* headerBuffPtr;
      UString request(U_CAPACITY), params(U_CAPACITY);
      int nameLen, valueLen, headerLen, byte_to_read, i, n;

      (void) request.append((const char*)&beginRecord, sizeof(FCGI_BeginRequestRecord));

      // Set environment for the FCGI application server

      UString environment = UHTTP::getCGIEnvironment(true);

      if (environment.empty())
         {
         UHTTP::setHTTPBadRequest();

         U_RETURN(U_PLUGIN_HANDLER_ERROR);
         }

      n = u_split(U_STRING_TO_PARAM(environment), envp, 0);

      U_INTERNAL_ASSERT_MINOR(n, 128)

      for (i = 0; i < n; ++i)
         {
         equalPtr = strchr(envp[i], '=');

         U_INTERNAL_ASSERT_POINTER(equalPtr)

          nameLen = (equalPtr - envp[i]);
         valueLen = u__strlen(++equalPtr);

         U_INTERNAL_ASSERT_MAJOR( nameLen, 0)
         U_INTERNAL_ASSERT_MAJOR(valueLen, 0)

         // Builds a name-value pair header from the name length and the value length

         headerBuffPtr = headerBuff;

         if (nameLen < 0x80) *headerBuffPtr++ = (unsigned char) nameLen;
         else
            {
            *headerBuffPtr++ = (unsigned char) ((nameLen >> 24) | 0x80);
            *headerBuffPtr++ = (unsigned char)  (nameLen >> 16);
            *headerBuffPtr++ = (unsigned char)  (nameLen >>  8);
            *headerBuffPtr++ = (unsigned char)   nameLen;
            }

         if (valueLen < 0x80) *headerBuffPtr++ = (unsigned char) valueLen;
         else
            {
            *headerBuffPtr++ = (unsigned char) ((valueLen >> 24) | 0x80);
            *headerBuffPtr++ = (unsigned char)  (valueLen >> 16);
            *headerBuffPtr++ = (unsigned char)  (valueLen >>  8);
            *headerBuffPtr++ = (unsigned char)   valueLen;
            }

         headerLen = headerBuffPtr - headerBuff;

         U_INTERNAL_ASSERT_MAJOR(valueLen, 0)

         (void) params.append((const char*)headerBuff, headerLen);
         (void) params.append(envp[i], nameLen);
         (void) params.append(equalPtr, valueLen);
         }

      fill_FCGIBeginRequest(FCGI_PARAMS, params.size());

      (void) request.append((const char*)&beginRecord, FCGI_HEADER_LEN);
      (void) request.append(params);

      // maybe we have some data to put on stdin of cgi process (POST)

      U_INTERNAL_DUMP("UClientImage_Base::body(%u) = %.*S", UClientImage_Base::body->size(), U_STRING_TO_TRACE(*UClientImage_Base::body))

      size = UClientImage_Base::body->size();

      if (size)
         {
         U_INTERNAL_ASSERT(UHTTP::isHttpPOST())

         fill_FCGIBeginRequest(FCGI_PARAMS, 0);

         (void) request.append((const char*)&beginRecord, FCGI_HEADER_LEN);
         }

      fill_FCGIBeginRequest(FCGI_STDIN, size);

      (void) request.append((const char*)&beginRecord, FCGI_HEADER_LEN);

      if (size)
         {
         (void) request.append(*UClientImage_Base::body);

         fill_FCGIBeginRequest(FCGI_STDIN, 0);

         (void) request.append((const char*)&beginRecord, FCGI_HEADER_LEN);
         }

      // Send request and read fast cgi header+record

      if (connection->sendRequest(request, true) == false) goto err;

      if (fcgi_keep_conn == false)
         {
         /* The shutdown() tells the receiver the server is done sending data. No
          * more data is going to be send. More importantly, it doesn't close the
          * socket. At the socket layer, this sends a TCP/IP FIN packet to the receiver
          */

         if (connection->shutdown() == false) goto err;
         }

      pos = 0;

      while (true)
         {
         U_INTERNAL_DUMP("response.c_pointer(%u) = %#.*S", pos, 16, connection->response.c_pointer(pos))

         U_INTERNAL_ASSERT((connection->response.size() - pos) >= FCGI_HEADER_LEN)

         h = (FCGI_Header*) connection->response.c_pointer(pos);

         U_INTERNAL_DUMP("version = %C request_id = %u", h->version, ntohs(h->request_id))

         U_INTERNAL_ASSERT_EQUALS(h->version,    FCGI_VERSION_1)
         U_INTERNAL_ASSERT_EQUALS(h->request_id, htons((uint16_t)u_pid))

         h->content_length = ntohs(h->content_length);

         clength      = h->content_length + h->padding_length;
         byte_to_read = pos + FCGI_HEADER_LEN + clength - connection->response.size();

         U_INTERNAL_DUMP("pos = %u clength = %u response.size() = %u byte_to_read = %d", pos, clength, connection->response.size(), byte_to_read)

         if (byte_to_read > 0 &&
             connection->readResponse(byte_to_read) == false)
            {
            break;
            }

         // NB: connection->response can be resized...

         h = (FCGI_Header*) connection->response.c_pointer(pos);

         // Record fully read

         pos += FCGI_HEADER_LEN;

         // Process this fcgi record

         U_INTERNAL_DUMP("h->type = %C", h->type)

         switch (h->type)
            {
            case FCGI_STDOUT:
               {
               if (clength) (void) UClientImage_Base::wbuffer->append(connection->response.substr(pos, clength));
               }
            break;

            case FCGI_STDERR:
               (void) UFile::write(STDERR_FILENO, connection->response.c_pointer(pos), clength);
            break;

            case FCGI_END_REQUEST:
               {
               FCGI_EndRequestBody* body = (FCGI_EndRequestBody*)connection->response.c_pointer(pos);

               U_INTERNAL_DUMP("protocol_status = %C app_status = %u", body->protocol_status, ntohl(body->app_status))

               if (body->protocol_status == FCGI_REQUEST_COMPLETE)
                  {
                  U_INTERNAL_ASSERT_EQUALS(pos + clength, connection->response.size())

                  (void) UHTTP::processCGIOutput();

                  goto end;
                  }
               }
         // break; NB: intenzionale...

            // not  implemented
            case FCGI_UNKNOWN_TYPE:
            case FCGI_GET_VALUES_RESULT:
            default:
            goto err;
            }

         pos += clength;

         U_INTERNAL_DUMP("pos = %u response.size() = %u", pos, connection->response.size())

         if ((connection->response.size() - pos) < FCGI_HEADER_LEN &&
             connection->readResponse() == false)
            {
            break;
            }
         }

      goto end; // skip error...

err:  UHTTP::setHTTPInternalError();
end:  UHTTP::setHTTPRequestProcessed();

      // reset

      connection->clearData();

      if (fcgi_keep_conn == false &&
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

const char* UFCGIPlugIn::dump(bool reset) const
{
   *UObjectIO::os << "fcgi_keep_conn              " << fcgi_keep_conn         << '\n'
                  << "connection    (UClient_Base " << (void*)connection      << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
