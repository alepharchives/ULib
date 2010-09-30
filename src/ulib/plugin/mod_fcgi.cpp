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
#include <ulib/plugin/mod_fcgi.h>
#include <ulib/net/client/client.h>
#include <ulib/net/server/server.h>

#ifndef __MINGW32__
#  include <ulib/net/unixsocket.h>
#endif

// ---------------------------------------------------------------------------------------------------------------
// START Fast CGI stuff
// ---------------------------------------------------------------------------------------------------------------
/*
 * Number of bytes in a FCGI_Header. Future versions of the protocol
 * will not reduce this number.
 */
#define FCGI_HEADER_LEN  8

/*
 * Value for version component of FCGI_Header
 */
#define FCGI_VERSION_1           1

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
#define FCGI_NULL_REQUEST_ID     0

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
#define FCGI_KEEP_CONN  1

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

static void fill_header(FCGI_Header& h, u_char type, u_short content_length)
{
   U_TRACE(0, "UFCGIPlugIn::fill_header(%p,%C,%d)", &h, type, content_length)

// h.version        = FCGI_VERSION_1;
   h.type           = type;
// h.request_id     = htons((uint16_t)u_pid);
   h.content_length = htons(content_length);
// h.padding_length = padding_length;
// h.reserved       = 0;
}
// ---------------------------------------------------------------------------------------------------------------
// END Fast CGI stuff
// ---------------------------------------------------------------------------------------------------------------

U_CREAT_FUNC(UFCGIPlugIn)

UString* UFCGIPlugIn::str_FCGI_URI_MASK;
UString* UFCGIPlugIn::str_FCGI_KEEP_CONN;

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

UFCGIPlugIn::~UFCGIPlugIn()
{
   U_TRACE_UNREGISTER_OBJECT(0, UFCGIPlugIn)

   if (connection) delete connection;
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
   // USE_IPV6       flag to indicate use of ipv6
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
      fcgi_uri_mask  = cfg[*str_FCGI_URI_MASK];
      fcgi_keep_conn = cfg.readBoolean(*str_FCGI_KEEP_CONN);

      connection = U_NEW(UClient_Base(&cfg));
      }

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

int UFCGIPlugIn::handlerInit()
{
   U_TRACE(1, "UFCGIPlugIn::handlerInit()")

   if (connection &&
       fcgi_uri_mask.empty() == false)
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
         U_SRV_LOG_VAR("connection to the fastcgi-backend %.*S accepted", U_STRING_TO_TRACE(connection->host_port));

         U_SRV_LOG_MSG("initialization of plugin success");

         U_RETURN(U_PLUGIN_HANDLER_GO_ON);
         }
      }

   U_SRV_LOG_MSG("initialization of plugin FAILED");

   U_RETURN(U_PLUGIN_HANDLER_ERROR);
}

// Connection-wide hooks

int UFCGIPlugIn::handlerRequest()
{
   U_TRACE(0, "UFCGIPlugIn::handlerRequest()")

   if (UHTTP::isHTTPRequestAlreadyProcessed() == false &&
       u_dosmatch_with_OR(U_HTTP_URI_TO_PARAM, U_STRING_TO_PARAM(fcgi_uri_mask), 0))
      {
      // Set FCGI_BEGIN_REQUEST

      static bool init_fcgi_begin_request;
      static FCGI_BeginRequestRecord beginRecord;

      if (init_fcgi_begin_request == false)
         {
         init_fcgi_begin_request       = true;

         beginRecord.header.version    = FCGI_VERSION_1;
         beginRecord.header.request_id = htons((uint16_t)u_pid);

         beginRecord.body.role         = htons(FCGI_RESPONDER);
         beginRecord.body.flags        = fcgi_keep_conn;
         }

      fill_header(beginRecord.header, FCGI_BEGIN_REQUEST, sizeof(FCGI_BeginRequestBody));

      FCGI_Header* h;
      char* equalPtr;
      char* envp[128];
      uint32_t clength, pos, size;
      unsigned char  headerBuff[8];
      unsigned char* headerBuffPtr;
      int nameLen, valueLen, headerLen, byte_to_read, i, n;
      UString request(U_CAPACITY), environment(U_CAPACITY), params(U_CAPACITY), response;

      (void) request.append((const char*)&beginRecord, sizeof(FCGI_BeginRequestRecord));

      // Set environment for the FCGI application server

      environment = UHTTP::getCGIEnvironment() + *UHTTP::penvironment;

      n = u_split(U_STRING_TO_PARAM(environment), envp, 0);

      U_INTERNAL_ASSERT_MINOR(n, 128)

      for (i = 0; i < n; ++i)
         {
         equalPtr = strchr(envp[i], '=');

         U_INTERNAL_ASSERT_POINTER(equalPtr)

          nameLen = (equalPtr - envp[i]);
         valueLen = strlen(++equalPtr);

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

      fill_header(beginRecord.header, FCGI_PARAMS, params.size());

      (void) request.append((const char*)&beginRecord, FCGI_HEADER_LEN);
      (void) request.append(params);

      size = UClientImage_Base::body->size();

      if (size)
         {
         fill_header(beginRecord.header, FCGI_PARAMS, 0);

         (void) request.append((const char*)&beginRecord, FCGI_HEADER_LEN);
         }

      fill_header(beginRecord.header, FCGI_STDIN, size);

      (void) request.append((const char*)&beginRecord, FCGI_HEADER_LEN);

      if (size)
         {
         (void) request.append(*UClientImage_Base::body);

         fill_header(beginRecord.header, FCGI_STDIN, 0);

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

      pos      = 0;
      response = connection->getResponse();

      while (true)
         {
         U_INTERNAL_DUMP("response.c_pointer(%u) = %#.*S", pos, 16, response.c_pointer(pos))

         U_INTERNAL_ASSERT((response.size() - pos) >= FCGI_HEADER_LEN)

         h = (FCGI_Header*) response.c_pointer(pos);

         U_INTERNAL_DUMP("version = %C request_id = %u", h->version, ntohs(h->request_id))

         U_INTERNAL_ASSERT_EQUALS(h->version,    FCGI_VERSION_1)
         U_INTERNAL_ASSERT_EQUALS(h->request_id, htons((uint16_t)u_pid))

         h->content_length = ntohs(h->content_length);

         clength      = h->content_length + h->padding_length;
         byte_to_read = pos + FCGI_HEADER_LEN + clength - response.size();

         U_INTERNAL_DUMP("pos = %u clength = %u response.size() = %u byte_to_read = %d", pos, clength, response.size(), byte_to_read)

         if (byte_to_read > 0 &&
             connection->readResponse(byte_to_read) == false)
            {
            break;
            }

         // Record fully read

         pos += FCGI_HEADER_LEN;

         // Process this fcgi record

         U_INTERNAL_DUMP("h->type = %C", h->type)

         switch (h->type)
            {
            case FCGI_STDOUT:
               {
               if (clength) (void) UClientImage_Base::wbuffer->append(response.substr(pos, clength));
               }
            break;

            case FCGI_STDERR:
               (void) UFile::write(STDERR_FILENO, response.c_pointer(pos), clength);
            break;

            case FCGI_END_REQUEST:
               {
               FCGI_EndRequestBody* body = (FCGI_EndRequestBody*) response.c_pointer(pos);

               U_INTERNAL_DUMP("protocol_status = %C app_status = %u", body->protocol_status, ntohl(body->app_status))

               if (body->protocol_status == FCGI_REQUEST_COMPLETE)
                  {
                  U_INTERNAL_ASSERT_EQUALS(pos + clength, response.size())

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

         U_INTERNAL_DUMP("pos = %u response.size() = %u", pos, response.size())

         if ((response.size() - pos) < FCGI_HEADER_LEN &&
             connection->readResponse() == false)
            {
            break;
            }
         }

      goto end; // skip error...

err:  UHTTP::setHTTPInternalError();
end:  UHTTP::setHTTPRequestProcessed();
      }

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

int UFCGIPlugIn::handlerReset()
{
   U_TRACE(0, "UFCGIPlugIn::handlerReset()")

   connection->clearData();

   if (fcgi_keep_conn == false &&
       connection->isConnected())
      {
      connection->close();
      }

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UFCGIPlugIn::dump(bool reset) const
{
   *UObjectIO::os << "fcgi_keep_conn              " << fcgi_keep_conn         << '\n'
                  << "fcgi_uri_mask (UString      " << (void*)&fcgi_uri_mask  << ")\n"
                  << "connection    (UClient_Base " << (void*)connection      << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
