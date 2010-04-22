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
#include <ulib/net/unixsocket.h>
#include <ulib/plugin/mod_fcgi.h>
#include <ulib/net/client/client.h>
#include <ulib/net/server/server.h>

// ---------------------------------------------------------------------------------------------------------------
// START Fast CGI stuff
// ---------------------------------------------------------------------------------------------------------------

typedef struct {
   u_char  version;
   u_char  type;
   u_short request_id;
   u_short content_length;
   u_char  padding_length;
   u_char  reserved;
} FCGI_Header;

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
   u_short role;
   u_char  flags;
   u_char  reserved[5];
} FCGI_BeginRequestBody;

typedef struct {
   FCGI_Header           header;
   FCGI_BeginRequestBody body;
} FCGI_BeginRequestRecord;

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

// Initialize a fast CGI header record

static void* fill_header(void* buf, u_char type, u_short content_length, u_char padding_length)
{
   U_TRACE(0, "UFCGIPlugIn::fill_header(%p,%C,%d,%C)", buf, type, content_length, padding_length)

   U_INTERNAL_ASSERT_POINTER(buf)

   FCGI_Header* h = (FCGI_Header*)buf;

   h->version        = FCGI_VERSION_1;
   h->type           = type;
   h->request_id     = htons(u_pid);
   h->content_length = htons(content_length);
   h->padding_length = padding_length;
// h->reserved       = 0;

   return ((char*)buf) + FCGI_HEADER_LEN;
}

static void fill_begin_request(void* buf, u_int role, u_char flags)
{
   U_TRACE(0, "UFCGIPlugIn::fill_begin_request(%p,%u,%C)", buf, role, flags)

   FCGI_BeginRequestBody* body = (FCGI_BeginRequestBody*) fill_header(buf, FCGI_BEGIN_REQUEST, sizeof(FCGI_BeginRequestBody), 0);

   body->role  = htons(role);
   body->flags = flags;

// (void) U_SYSCALL(memset, "%p,%d,%u", body->reserved,  0, sizeof(body->reserved));
}

// ---------------------------------------------------------------------------------------------------------------
// END Fast CGI stuff
// ---------------------------------------------------------------------------------------------------------------

U_CREAT_FUNC(UFCGIPlugIn)

UString* UFCGIPlugIn::str_FCGI_URI_MASK;

void UFCGIPlugIn::str_allocate()
{
   U_TRACE(0, "UFCGIPlugIn::str_allocate()")

   U_INTERNAL_ASSERT_EQUALS(str_FCGI_URI_MASK,0)

   static ustringrep stringrep_storage[] = {
      { U_STRINGREP_FROM_CONSTANT("FCGI_URI_MASK") }
   };

   U_NEW_ULIB_OBJECT(str_FCGI_URI_MASK, U_STRING_FROM_STRINGREP_STORAGE(0));
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
   // FCGI_URI_MASK mask (DOS regexp) of uri type that send request via FCGI (*.php)
   //
   // NAME_SOCKET   file name for the fcgi socket
   //
   // USE_IPV6      flag to indicate use of ipv6
   // SERVER        host name or ip address for the fcgi host
   // PORT          port number             for the fcgi host
   //
   // RES_TIMEOUT   timeout for response from FCGI
   //
   // LOG_FILE      locations for file log (use server log if exist)
   // ------------------------------------------------------------------------------------------

   if (cfg.loadTable())
      {
      fcgi_uri_mask = cfg[*str_FCGI_URI_MASK];

      connection = U_NEW(UClient_Base(&cfg));
      }

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

int UFCGIPlugIn::handlerInit()
{
   U_TRACE(1, "UFCGIPlugIn::handlerInit()")

   if (connection)
      {
      connection->socket = connection->port ? (USocket*)U_NEW(UTCPSocket(connection->bIPv6))
                                            : (USocket*)U_NEW(UUnixSocket);

      if (connection->connect())
         {
         U_SRV_LOG_VAR("connection to the fastcgi-backend %.*S accepted", U_STRING_TO_TRACE(connection->host_port));

         U_SRV_LOG_MSG("initialization of plugin success");

         U_RETURN(U_PLUGIN_HANDLER_GO_ON);
         }

      U_SRV_LOG_MSG(connection->getResponseData());
      }

   U_SRV_LOG_MSG("initialization of plugin FAILED");

   U_RETURN(U_PLUGIN_HANDLER_ERROR);
}

// Connection-wide hooks

int UFCGIPlugIn::handlerRequest()
{
   U_TRACE(0, "UFCGIPlugIn::handlerRequest()")

   if (fcgi_uri_mask.empty() ||
       u_dosmatch_with_OR(U_HTTP_URI_TO_PARAM, U_STRING_TO_PARAM(fcgi_uri_mask), 0) == false)
      {
      U_RETURN(U_PLUGIN_HANDLER_GO_ON);
      }

   if (connection->port == 0 &&
       UHTTP::checkHTTPRequest() == false)
      {
      U_RETURN(U_PLUGIN_HANDLER_FINISHED);
      }

   FCGI_Header* h;
   char* equalPtr;
   char* envp[128];
   uint32_t clength, pos;
   unsigned char  headerBuff[8];
   unsigned char* headerBuffPtr;
   int nameLen, valueLen, headerLen, byte_to_read, i, n;
   UString request(U_CAPACITY), environment(U_CAPACITY), params(U_CAPACITY), response;

   // Send FCGI_BEGIN_REQUEST

   static FCGI_BeginRequestRecord beginRecord;

   fill_begin_request(&beginRecord, FCGI_RESPONDER, FCGI_KEEP_CONN);

   (void) request.replace((const char*)&beginRecord, sizeof(FCGI_BeginRequestRecord));

   // Send environment to the FCGI application server

   environment = *UHTTP::penvironment + UHTTP::getCGIEnvironment(false);

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

   (void) fill_header(&beginRecord, FCGI_PARAMS, params.size(), 0);

   (void) request.append((const char*)&beginRecord, FCGI_HEADER_LEN);
   (void) request.append(params);

   (void) fill_header(&beginRecord, FCGI_STDIN, UClientImage_Base::body->size(), 0);

   (void) request.append((const char*)&beginRecord, FCGI_HEADER_LEN);
   (void) request.append(*UClientImage_Base::body);

   if (connection->sendRequest(request) == false) goto error;

   // Read fast cgi header+record

   pos          = 0;
   byte_to_read = U_SINGLE_READ;

   while (true)
      {
      if (connection->readResponse(byte_to_read) == false) goto error;

      response = connection->getResponse();

loop:
      U_INTERNAL_ASSERT_MAJOR(response.size() - pos, FCGI_HEADER_LEN)

      h = (FCGI_Header*) response.c_pointer(pos);

      U_INTERNAL_ASSERT_EQUALS(h->version, FCGI_VERSION_1)

   // h->request_id     = ntohs(h->request_id);
      h->content_length = ntohs(h->content_length);

      clength      = h->content_length + h->padding_length;
      byte_to_read = clength - response.size() - pos - FCGI_HEADER_LEN;

      U_INTERNAL_DUMP("clength = %u pos = %u response.size() = %u byte_to_read = %d", clength, pos, response.size(), byte_to_read)

      if (byte_to_read > 0) continue;

      // Record fully read
      // Process this fcgi record

      pos += FCGI_HEADER_LEN;

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
            (void) UHTTP::processCGIOutput();
            }
         goto end;

         // not  implemented
         case FCGI_UNKNOWN_TYPE:
         case FCGI_GET_VALUES_RESULT:
         default:
         goto error;
         }

      pos += clength;

      if ((response.size() - pos) >= FCGI_HEADER_LEN) goto loop;

      byte_to_read = U_SINGLE_READ;
      }

error:
   UHTTP::setHTTPInternalError(); // set internal error response...

end:
   U_RETURN(U_PLUGIN_HANDLER_FINISHED);
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UFCGIPlugIn::dump(bool reset) const
{
   *UObjectIO::os << "fcgi_uri_mask (UString      " << (void*)&fcgi_uri_mask  << ")\n"
                  << "connection    (UClient_Base " << (void*)connection      << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
