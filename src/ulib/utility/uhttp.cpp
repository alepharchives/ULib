// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    uhttp.cpp - HTTP utility
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/date.h>
#include <ulib/file.h>
#include <ulib/command.h>
#include <ulib/tokenizer.h>
#include <ulib/mime/entity.h>
#include <ulib/utility/uhttp.h>
#include <ulib/utility/base64.h>
#include <ulib/utility/services.h>
#include <ulib/net/server/server.h>
#include <ulib/utility/socket_ext.h>
#include <ulib/utility/string_ext.h>

UFile*            UHTTP::file;
UString*          UHTTP::tmpdir;
UString*          UHTTP::qcontent;
uhttpheader       UHTTP::http_info;
const char*       UHTTP::ptrC;
const char*       UHTTP::ptrH;
UMimeMultipart*   UHTTP::formMulti;
UVector<UString>* UHTTP::form_name_value;

const UString*    UHTTP::str_frm_response;
const UString*    UHTTP::str_frm_forbidden;
const UString*    UHTTP::str_frm_not_found;
const UString*    UHTTP::str_frm_moved_temp;
const UString*    UHTTP::str_frm_bad_request;
const UString*    UHTTP::str_frm_internal_error;
const UString*    UHTTP::str_frm_service_unavailable;

void UHTTP::str_allocate()
{
   U_TRACE(0, "UHTTP::str_allocate()")

   U_INTERNAL_ASSERT_EQUALS(str_frm_response,0)
   U_INTERNAL_ASSERT_EQUALS(str_frm_forbidden,0)
   U_INTERNAL_ASSERT_EQUALS(str_frm_not_found,0)
   U_INTERNAL_ASSERT_EQUALS(str_frm_moved_temp,0)
   U_INTERNAL_ASSERT_EQUALS(str_frm_bad_request,0)
   U_INTERNAL_ASSERT_EQUALS(str_frm_internal_error,0)
   U_INTERNAL_ASSERT_EQUALS(str_frm_service_unavailable,0)

   static ustringrep stringrep_storage[] = {
   { U_STRINGREP_FROM_CONSTANT("HTTP/1.%c %d %s\r\n"
                               "Server: ULib/1.0\r\n"
                               "%.*s"
                               "%.*s") },
   { U_STRINGREP_FROM_CONSTANT("HTTP/1.%c 403 Forbidden\r\n"
                               "Date: %D\r\n"
                               "Server: ULib/1.0\r\n"
                               "Connection: close\r\n"
                               "Content-Type: " U_CTYPE_HTML "\r\n"
                               "Content-Length: %u\r\n"
                               "\r\n"
                               "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\r\n"
                               "<html><head>\r\n"
                               "<title>403 Forbidden</title>\r\n"
                               "</head><body>\r\n"
                               "<h1>Forbidden</h1>\r\n"
                               "<p>You don't have permission to access %.*s on this server.<br />\r\n"
                               "</p>\r\n"
                               "<hr>\r\n"
                               "<address>ULib Server</address>\r\n"
                               "</body></html>\r\n") },
   { U_STRINGREP_FROM_CONSTANT("HTTP/1.%c 404 Not Found\r\n"
                               "Date: %D\r\n"
                               "Server: ULib/1.0\r\n"
                               "Connection: close\r\n"
                               "Content-Type: " U_CTYPE_HTML "\r\n"
                               "Content-Length: %u\r\n"
                               "\r\n"
                               "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\r\n"
                               "<html><head>\r\n"
                               "<title>404 Not Found</title>\r\n"
                               "</head><body>\r\n"
                               "<h1>Not Found</h1>\r\n"
                               "<p>The requested URL %.*s was not found on this server.<br />\r\n"
                               "</p>\r\n"
                               "<hr>\r\n"
                               "<address>ULib Server</address>\r\n"
                               "</body></html>\r\n") },
   { U_STRINGREP_FROM_CONSTANT("HTTP/1.%c 302 Found\r\n"
                               "Server: ULib/1.0\r\n"
                               "%.*s"
                               "Location: %.*s\r\n"
                               "Content-Type: " U_CTYPE_HTML "\r\n"
                               "Content-Length: %u\r\n"
                               "\r\n"
                               "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\r\n"
                               "<html><head>\r\n"
                               "<title>302 Found</title>\r\n"
                               "</head><body>\r\n"
                               "<h1>Found</h1>\r\n"
                               "<p>The document has moved <a href=\"%.*s\">here</a>.<br />\r\n"
                               "</p>\r\n"
                               "<hr>\r\n"
                               "<address>ULib Server</address>\r\n"
                               "</body></html>\r\n") },
   { U_STRINGREP_FROM_CONSTANT("<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\r\n"
                               "<html><head>\r\n"
                               "<title>400 Bad Request</title>\r\n"
                               "</head><body>\r\n"
                               "<h1>Bad Request</h1>\r\n"
                               "<p>Your browser sent a request that this server could not understand.<br />\r\n"
                               "</p>\r\n"
                               "<hr>\r\n"
                               "<address>ULib Server</address>\r\n"
                               "</body></html>\r\n") },
   { U_STRINGREP_FROM_CONSTANT("<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\r\n"
                               "<html><head>\r\n"
                               "<title>500 Internal Server Error</title>\r\n"
                               "</head><body>\r\n"
                               "<h1>Internal Server Error</h1>\r\n"
                               "<p>The server encountered an internal error or misconfiguration and was unable "
                               "to complete your request.</p>\r\n"
                               "<p>Please contact the server administrator, and inform them of the time the error "
                               "occurred, and anything you might have done that may have caused the error.</p>\r\n"
                               "<p>More information about this error may be available in the server error log.</p>\r\n"
                               "<hr>\r\n"
                               "<address>ULib Server</address>\r\n"
                               "</body></html>\r\n") },
   { U_STRINGREP_FROM_CONSTANT("<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\r\n"
                               "<html><head>\r\n"
                               "<title>503 Service Unavailable</title>\r\n"
                               "</head><body>\r\n"
                               "<h1>Service Unavailable</h1>\r\n"
                               "<p>Sorry, the service you requested is not available at this moment. "
                               "Please contact the server administrator and inform them about this.<br />\r\n"
                               "</p>\r\n"
                               "<hr>\r\n"
                               "<address>ULib Server</address>\r\n"
                               "</body></html>\r\n") }
   };

   U_NEW_ULIB_OBJECT(str_frm_response,             U_STRING_FROM_STRINGREP_STORAGE(0));
   U_NEW_ULIB_OBJECT(str_frm_forbidden,            U_STRING_FROM_STRINGREP_STORAGE(1));
   U_NEW_ULIB_OBJECT(str_frm_not_found,            U_STRING_FROM_STRINGREP_STORAGE(2));
   U_NEW_ULIB_OBJECT(str_frm_moved_temp,           U_STRING_FROM_STRINGREP_STORAGE(3));
   U_NEW_ULIB_OBJECT(str_frm_bad_request,          U_STRING_FROM_STRINGREP_STORAGE(4));
   U_NEW_ULIB_OBJECT(str_frm_internal_error,       U_STRING_FROM_STRINGREP_STORAGE(5));
   U_NEW_ULIB_OBJECT(str_frm_service_unavailable,  U_STRING_FROM_STRINGREP_STORAGE(6));
}

/* read HTTP message
======================================================================================
Read the request line and attached headers. A typical http request will take the form:
======================================================================================
GET / HTTP/1.1
Host: 127.0.0.1
User-Agent: Mozilla/5.0 (compatible; Konqueror/3.1; Linux; it, en_US, en)
Accept-Encoding: x-gzip, x-deflate, gzip, deflate, identity
Accept-Charset: iso-8859-1, utf-8;q=0.5, *;q=0.5
Accept-Language: it, en
Connection: Keep-Alive
<empty line>
======================================================================================
Read the result line and attached headers. A typical http response will take the form:
======================================================================================
HTTP/1.1 200 OK
Date: Wed, 25 Oct 2000 16:54:02 GMT
Age: 57718
Server: Apache/1.3b5
Last-Modified: Sat, 23 Sep 2000 15:00:57 GMT
Accept-Ranges: bytes
Etag: "122b12-2d8c-39ccc5a9"
Content-Type: text/html
Content-Length: 11660
<empty line>
.......<the data>
====================================================================================
*/

bool UHTTP::isHTTPRequest(const char* ptr)
{
   U_TRACE(0, "UHTTP::isHTTPRequest(%.*S)", 30, ptr)

   while (u_isspace(*ptr)) ++ptr; // skip space...

   unsigned char c = u_toupper(ptr[0]);

   if (c != 'G' && // GET
       c != 'P' && // POST
       c != 'H')   // HEAD
      {
      U_RETURN(false);
      }

   if (c == 'G') // GET
      {
      if (U_STRNCASECMP(ptr, "GET ")) U_RETURN(false);
      }
   else if (c == 'P') // POST
      {
      if (U_STRNCASECMP(ptr, "POST ")) U_RETURN(false);
      }
   else // HEAD
      {
      if (U_STRNCASECMP(ptr, "HEAD ")) U_RETURN(false);
      }

   U_RETURN(true);
}

bool UHTTP::scanfHTTPHeader(const char* ptr)
{
   U_TRACE(0, "UHTTP::scanfHTTPHeader(%.*S)", 30, ptr)

   /**
    * Check if HTTP response or request
    *
    * The default is GET for input requests and POST for output requests
    *
    * Other possible alternatives are:
    *  - HEAD
    *  ---- NOT implemented -----
    *  - PUT
    *  - TRACE
    *  - DELETE
    *  - OPTIONS
    *  --------------------------
    *
    * See http://ietf.org/rfc/rfc2616.txt for further information about HTTP request methods
    **/

   while (u_isspace(*ptr)) ++ptr; // RFC 2616 4.1 "servers SHOULD ignore any empty line(s) received where a Request-Line is expected."

   unsigned char c = u_toupper(ptr[0]);

   if (c != 'G' && // GET
       c != 'P' && // POST
       c != 'H')   // HEAD or response
      {
      U_RETURN(false);
      }

   // try to parse the request line: GET / HTTP/1.n

   const char* start = http_info.method = ptr;

   if (c == 'G') // GET
      {
      http_info.method_len  = 3;
      http_info.method_type = HTTP_GET;

      U_INTERNAL_ASSERT_EQUALS(U_STRNCASECMP(http_info.method, "GET "), 0)
      }
   else if (c == 'P') // POST
      {
      http_info.method_len  = 4;
      http_info.method_type = HTTP_POST;

      U_INTERNAL_ASSERT_EQUALS(U_STRNCASECMP(http_info.method, "POST "), 0)
      }
   else // HEAD or response
      {
      if (ptr[1] == 'T')
         {
         // try to parse the response line: HTTP/1.n nnn <ssss>

         if (U_STRNCMP(ptr, "HTTP/1.")) U_RETURN(false);

         ptr += U_CONSTANT_SIZE("HTTP/1.") + 2;

         http_info.nResponseCode = strtol(ptr, (char**)&ptr, 10);

         U_INTERNAL_DUMP("nResponseCode = %d", http_info.nResponseCode)

         goto end;
         }

      http_info.method_len  = 4;
      http_info.method_type = HTTP_HEAD;

      U_INTERNAL_ASSERT_EQUALS(U_STRNCASECMP(http_info.method, "HEAD "), 0)
      }

   U_INTERNAL_DUMP("method = %.*S method_type = %u", U_HTTP_METHOD_TO_TRACE, http_info.method_type)

   ptr += http_info.method_len;

   while (u_isspace(*++ptr)) {} // RFC 2616 19.3 "[servers] SHOULD accept any amount of SP or HT characters between [Request-Line] fields"

   http_info.uri = ptr;

   while (true)
      {
      c = *ptr;

      if (c == ' ') break;

      /* check uri for invalid characters (NB: \n can happen because openssl base64...) */

      if (c <=  32 ||
          c == 127 ||
          c == 255)
         {
         U_WARNING("invalid character %C in URI %.*S", c, ptr - http_info.uri, http_info.uri);

         U_RETURN(false);
         }

      if (c == '?')
         {
         http_info.uri_len = ptr - http_info.uri;
         http_info.query   = ++ptr;

         continue;
         }

      ++ptr;
      }

   if (http_info.query)
      {
      http_info.query_len = ptr - http_info.query;
      http_info.query    -= (ptrdiff_t)start;

      U_INTERNAL_DUMP("query = %.*S", U_HTTP_QUERY_TO_TRACE)
      }
   else
      {
      http_info.uri_len = ptr - http_info.uri;
      http_info.uri    -= (ptrdiff_t)start;
      }

   U_INTERNAL_DUMP("uri = %.*S", U_HTTP_URI_TO_TRACE)

   while (u_isspace(*++ptr)) {} // RFC 2616 19.3 "[servers] SHOULD accept any amount of SP or HT characters between [Request-Line] fields"

   if (U_STRNCMP(ptr, "HTTP/1.")) U_RETURN(false);

   ptr += U_CONSTANT_SIZE("HTTP/1.");

   http_info.version = ptr[0] - '0';

   U_INTERNAL_DUMP("http_info.version = %u", http_info.version)

end:
   for (c = u_line_terminator[0]; *ptr != c; ++ptr) {}

   http_info.startHeader = ptr - start;

   U_INTERNAL_DUMP("startHeader = %u", http_info.startHeader)

   U_INTERNAL_ASSERT(U_STRNCMP(ptr, U_LF)   == 0 ||
                     U_STRNCMP(ptr, U_CRLF) == 0)

   U_RETURN(true);
}

bool UHTTP::readHTTPHeader(USocket* s, UString& rbuffer)
{
   U_TRACE(0, "UHTTP::readHTTPHeader(%p,%.*S)", s, U_STRING_TO_TRACE(rbuffer))

   U_ASSERT(rbuffer.empty())
   U_INTERNAL_ASSERT(s->isConnected())
   U_INTERNAL_ASSERT_EQUALS(http_info.szHeader,0)
   U_INTERNAL_ASSERT_EQUALS(http_info.endHeader,0)
   U_INTERNAL_ASSERT_EQUALS(http_info.startHeader,0)

   const char* ptr;
   int timeoutMS = -1;
   uint32_t endHeader = 0, count = 0;

start:
   U_INTERNAL_DUMP("startHeader = %u", http_info.startHeader)

   if ((endHeader == U_NOT_FOUND ||
        rbuffer.size() <= http_info.startHeader) &&
       USocketExt::read(s, rbuffer, U_SINGLE_READ, timeoutMS) == false)
      {
      U_RETURN(false);
      }

   ptr       = rbuffer.c_pointer(http_info.startHeader);
   endHeader = u_findEndHeader(ptr, rbuffer.remain(ptr));

   if (endHeader == U_NOT_FOUND)
      {
      if (rbuffer.isBinary()) U_RETURN(false);

      // NB: attacked by a "slow loris"... http://lwn.net/Articles/337853/

      if (count++ > 5) U_RETURN(false);

      timeoutMS = 3 * 1000;

      goto start;
      }

   // NB: endHeader comprende anche la blank line...

   endHeader += http_info.startHeader;

   U_INTERNAL_DUMP("endHeader = %u", endHeader)

   // NB: http_info.startHeader is needed for loop...

   if (scanfHTTPHeader(rbuffer.c_pointer(http_info.endHeader)) == false) U_RETURN(false);

   http_info.startHeader += http_info.endHeader;

   U_INTERNAL_DUMP("startHeader = %u", http_info.startHeader)

   // check if HTTP response line: HTTP/1.n 100 Continue

   if (http_info.nResponseCode == HTTP_CONTINUE)
      {
      /*
      --------------------------------------------------------------------------------------------------------
      During the course of an HTTP 1.1 client sending a request to a server, the server might respond with
      an interim "100 Continue" response. This means the server has received the first part of the request,
      and can be used to aid communication over slow links. In any case, all HTT 1.1 clients must handle the
      100 response correctly (perhaps by just ignoring it). The "100 Continue" response is structured like
      any HTTP response, i.e. consists of a status line, optional headers, and a blank line. Unlike other
      responses, it is always followed by another complete, final response. Example:
      --------------------------------------------------------------------------------------------------------
      HTTP/1.0 100 Continue
      [blank line here]
      HTTP/1.0 200 OK
      Date: Fri, 31 Dec 1999 23:59:59 GMT
      Content-Type: text/plain
      Content-Length: 42
      some-footer: some-value
      another-footer: another-value
      [blank line here]
      abcdefghijklmnoprstuvwxyz1234567890abcdef
      --------------------------------------------------------------------------------------------------------
      To handle this, a simple HTTP 1.1 client might read one response from the socket;
      if the status code is 100, discard the first response and read the next one instead.
      --------------------------------------------------------------------------------------------------------
      */

      U_INTERNAL_ASSERT(U_STRNCMP(rbuffer.c_pointer(http_info.startHeader), U_LF2)   == 0 ||
                        U_STRNCMP(rbuffer.c_pointer(http_info.startHeader), U_CRLF2) == 0)

      http_info.startHeader += u_line_terminator_len * 2;
      http_info.endHeader    = http_info.startHeader;

      goto start;
      }

   http_info.startHeader += u_line_terminator_len;
   http_info.endHeader    = endHeader; // NB: endHeader comprende anche la blank line...
   http_info.szHeader     = endHeader - http_info.startHeader;

   U_INTERNAL_DUMP("szHeader = %u startHeader = %.*S endHeader = %.*S", http_info.szHeader,
                        20, rbuffer.c_pointer(http_info.startHeader),
                        20, rbuffer.c_pointer(http_info.endHeader))

   U_RETURN(true);
}

bool UHTTP::readHTTPBody(USocket* s, UString& rbuffer, UString& body)
{
   U_TRACE(0, "UHTTP::readHTTPBody(%p,%.*S,%.*S)", s, U_STRING_TO_TRACE(rbuffer), U_STRING_TO_TRACE(body))

   U_ASSERT(body.empty())
   U_INTERNAL_ASSERT(s->isConnected())
   U_INTERNAL_ASSERT_MAJOR(http_info.szHeader,0)

   uint32_t pos,
            count          = 0,
            chunk          = U_NOT_FOUND,
            skip           = (http_info.method - rbuffer.data()),
            body_byte_read = (rbuffer.size() - http_info.endHeader);

   U_INTERNAL_DUMP("skip = %u", skip)

start:
   U_INTERNAL_DUMP("rbuffer.size() = %u body_byte_read = %u", rbuffer.size(), body_byte_read)

   // NB: clength may have value... see UMimeEntity::readBody()

   if (http_info.clength == 0)
      {
      pos = rbuffer.find(*USocket::str_content_length, http_info.startHeader, http_info.szHeader);

      if (pos != U_NOT_FOUND) http_info.clength = (uint32_t) strtoul(rbuffer.c_pointer(pos + USocket::str_content_length->size() + 2), 0, 0);
      }

   U_INTERNAL_DUMP("Content-Length = %u", http_info.clength)

   if (http_info.clength)
      {
      if (http_info.clength > body_byte_read)
         {
         UString* pbuffer;

         if (http_info.clength > (64 * 1024 * 1024) && // 64M
             UFile::mkTempStorage(body, http_info.clength))
            {
            (void) U_SYSCALL(memcpy, "%p,%p,%u", body.data(), rbuffer.c_pointer(http_info.endHeader), body_byte_read);

            body.size_adjust(body_byte_read);

            pbuffer = &body;
            }
         else
            {
            pbuffer = &rbuffer;
            }

         if (USocketExt::read(s, *pbuffer, http_info.clength - body_byte_read, 3 * 1000) == false) U_RETURN(false);

         U_INTERNAL_DUMP("pbuffer = %.*S", U_STRING_TO_TRACE(*pbuffer))
         }

      body_byte_read = 0;
      }
   else
      {
      chunk = U_STRING_FIND_EXT(rbuffer, http_info.startHeader, "Transfer-Encoding: chunked", http_info.szHeader);

      U_INTERNAL_DUMP("chunk = %u", chunk)

      // HTTP/1.1 compliance: no missing Content-Length on POST requests

      if (chunk == U_NOT_FOUND && http_info.method_type == HTTP_POST) U_RETURN(false);
      }

   if (chunk == U_NOT_FOUND)
      {
      if (body_byte_read                                                 &&
          isHTTPRequest(rbuffer.c_pointer(http_info.endHeader)) == false && // check if pipeline...
          USocketExt::read(s, rbuffer, U_SINGLE_READ, 3 * 1000))            // wait max 3 sec for other data...
         {
         // NB: attacked by a "slow loris"... http://lwn.net/Articles/337853/

         if (count++ > 5) U_RETURN(false);

         goto start; // retry search for 'Content-Length: ' or chunk...
         }

      if (body.empty() && http_info.clength) body = rbuffer.substr(http_info.endHeader, http_info.clength);
      }
   else
      {
      /* ----------------------------------------------------------------------------------------------------------
      If a server wants to start sending a response before knowing its total length (like with long script output),
      it might use the simple chunked transfer-encoding, which breaks the complete response into smaller chunks and
      sends them in series. You can identify such a response because it contains the "Transfer-Encoding: chunked"
      header. All HTTP 1.1 clients must be able to receive chunked messages. A chunked message body contains a
      series of chunks, followed by a line with a single "0" (zero), followed by optional footers (just like headers),
      and a blank line. Each chunk consists of two parts: a line with the size of the chunk data, in hex, possibly
      followed by a semicolon and extra parameters you can ignore (none are currently standard), and ending with
      CRLF. the data itself, followed by CRLF. An example:

      HTTP/1.1 200 OK
      Date: Fri, 31 Dec 1999 23:59:59 GMT
      Content-Type: text/plain
      Transfer-Encoding: chunked
      [blank line here]
      1a; ignore-stuff-here
      abcdefghijklmnopqrstuvwxyz
      10
      1234567890abcdef
      0
      some-footer: some-value
      another-footer: another-value
      [blank line here]

      Note the blank line after the last footer. The length of the text data is 42 bytes (1a + 10, in hex),
      and the data itself is abcdefghijklmnopqrstuvwxyz1234567890abcdef. The footers should be treated like
      headers, as if they were at the top of the response. The chunks can contain any binary data, and may
      be much larger than the examples here. The size-line parameters are rarely used, but you should at
      least ignore them correctly. Footers are also rare, but might be appropriate for things like checksums
      or digital signatures
      -------------------------------------------------------------------------------------------------------- */

      // manage buffered read

      const char* chunk_terminator;
      uint32_t    chunk_terminator_len;

      if (u_line_terminator_len == 1)
         {
         chunk_terminator     = U_LF2;
         chunk_terminator_len = U_CONSTANT_SIZE(U_LF2);
         }
      else
         {
         chunk_terminator     = U_CRLF2;
         chunk_terminator_len = U_CONSTANT_SIZE(U_CRLF2);
         }

      count = rbuffer.find(chunk_terminator, http_info.endHeader, chunk_terminator_len);

      if (count == U_NOT_FOUND) count = USocketExt::read(s, rbuffer, chunk_terminator, chunk_terminator_len);

      if (count == U_NOT_FOUND) U_RETURN(false);

      count += chunk_terminator_len; // NB: il messaggio comprende anche la blank line...

      U_INTERNAL_DUMP("count = %u", count)

      http_info.clength = (count - http_info.endHeader);

      U_INTERNAL_DUMP("http_info.clength = %u", http_info.clength)

      body.setBuffer(http_info.clength);

      uint32_t chunkSize;
      const char* inp = rbuffer.c_pointer(http_info.endHeader);
            char* out = body.data();

      while (true)
         {
         // Decode the hexadecimal chunk size into an understandable number

         chunkSize = strtol(inp, 0, 16);

         U_INTERNAL_DUMP("chunkSize = %u inp[0] = %C", chunkSize, inp[0])

         U_INTERNAL_ASSERT_DIFFERS(u_isxdigit(*inp),0)

         // The last chunk is followed by zero or more trailers, followed by a blank line

         if (chunkSize == 0)
            {
            body.size_adjust(body.distance(out));

            U_INTERNAL_DUMP("body = %.*S", U_STRING_TO_TRACE(body))

            break;
            }

         while (*inp++ != '\n') {} // discard the rest of the line

         (void) U_SYSCALL(memcpy, "%p,%p,%u", out, inp, chunkSize);

         inp += chunkSize + u_line_terminator_len;
         out += chunkSize;

         U_INTERNAL_ASSERT(inp <= (rbuffer.c_pointer(count)))
         }
      }

   // NB: request rbuffer may have changed a cause of resize...

   if (http_info.nResponseCode == 0 &&                // request...
       (http_info.method != rbuffer.c_pointer(skip))) // resize...
      {
      http_info.method = rbuffer.c_pointer(skip);
      }

   U_RETURN(true);
}

bool UHTTP::readHTTPRequest()
{
   U_TRACE(0, "UHTTP::readHTTPRequest()")

   if (http_info.method) resetHTTPInfo();

   if (readHTTPHeader(UClientImage_Base::socket, *UClientImage_Base::rbuffer) == false)
      {
      // manage buffered read (pipelining)

      USocketExt::pcount  = UClientImage_Base::rbuffer->size();
#  ifdef DEBUG
      USocketExt::pbuffer = UClientImage_Base::rbuffer->data();
#  endif

      U_INTERNAL_DUMP("size_message = %u pcount = %d pbuffer = %p", USocketExt::size_message, USocketExt::pcount, USocketExt::pbuffer)

      U_RETURN(false);
      }

   if (http_info.method_type)
      {
      // --------------------------------
      // check in header request for:
      // --------------------------------
      // "Host: ..."
      // "Connection: ..."
      // --------------------------------

      const char* p;
      unsigned char c;
      const char* ptr    = UClientImage_Base::rbuffer->data();
      uint32_t pos, pos1 = http_info.startHeader, pos2, l;

      while (pos1 < http_info.endHeader)
         {
      // U_INTERNAL_DUMP("rbuffer = %.*S", 80, rbuffer.c_pointer(pos1))

         pos2 = UClientImage_Base::rbuffer->find('\n', pos1);

         if (pos2 == U_NOT_FOUND) pos2 = http_info.endHeader;

         c = ptr[pos1];

         if (c == 'H' || // "Host: ..."
             c == 'C')   // "Connection: ..."
            {
            pos = UClientImage_Base::rbuffer->find(':', ++pos1);

            U_INTERNAL_ASSERT_MINOR(pos,pos2)

            p = ptr + pos1;
            l = pos - pos1;

            do { ++pos; } while (pos < http_info.endHeader && u_isspace(ptr[pos]));

            if (c == 'H' &&
                l == 3   &&
                U_SYSCALL(memcmp, "%S,%S,%u", p, ptrH, l) == 0)
               {
               http_info.host     = ptr  + pos;
               http_info.host_len = pos2 - pos -1;

               U_INTERNAL_DUMP("http_info.host = %.*S", U_HTTP_HOST_TO_TRACE)
               }
            else if (l == 9 &&
                     U_SYSCALL(memcmp, "%S,%S,%u", p, ptrC, l) == 0)
               {
               U_INTERNAL_DUMP("Connection: = %.*S", pos2 - pos - 1, UClientImage_Base::rbuffer->c_pointer(pos))

               U_INTERNAL_ASSERT_EQUALS(c, 'C')

               p = ptr + pos;

               if (U_MEMCMP(p, "close") == 0)
                  {
                  http_info.is_connection_close = U_YES;

                  U_INTERNAL_DUMP("http_info.is_connection_close = %d", http_info.is_connection_close);
                  }
               else if (U_STRNCASECMP(p, "keep-alive") == 0)
                  {
                  http_info.keep_alive = 1;

                  U_INTERNAL_DUMP("http_info.keep_alive = %d", http_info.keep_alive);
                  }
               }
            }

         pos1 = pos2 + 1;
         }

      if (http_info.method_type != HTTP_POST) goto end;
      }

   if (readHTTPBody(UClientImage_Base::socket, *UClientImage_Base::rbuffer, *UClientImage_Base::body) == false) U_RETURN(false);

end:
   // manage buffered read (pipelining)

   USocketExt::size_message = http_info.endHeader + http_info.clength;

   UClientImage_Base::checkForPipeline();

   U_RETURN(true);
}

const char* UHTTP::getHTTPHeaderValuePtr(const UString& name)
{
   U_TRACE(0, "UHTTP::getHTTPHeaderValuePtr(%.*S)", U_STRING_TO_TRACE(name))

   uint32_t header_line = UClientImage_Base::rbuffer->find(name, http_info.startHeader, http_info.szHeader);

   if (header_line == U_NOT_FOUND) header_line = UClientImage_Base::rbuffer->findnocase(name, http_info.startHeader, http_info.szHeader); 

   if (header_line == U_NOT_FOUND) U_RETURN((const char*)0);

   U_INTERNAL_DUMP("header_line = %.*S", 20, UClientImage_Base::rbuffer->c_pointer(header_line))

   const char* ptr_header_value = UClientImage_Base::rbuffer->c_pointer(header_line + name.size() + 2);

   U_RETURN(ptr_header_value);
}

// Accept-Language: en-us,en;q=0.5
// ----------------------------------------------------
// take only the first 2 character (it, en, de fr, ...)

const char* UHTTP::getAcceptLanguage()
{
   U_TRACE(0, "UHTTP::getAcceptLanguage()")

   const char* ptr = getHTTPHeaderValuePtr(*USocket::str_accept_language);

   const char* accept_language = (ptr ? ptr : "en");

   U_INTERNAL_DUMP("accept_language = %.2S", ptr)

   U_RETURN_POINTER(accept_language,const char);
}

const char* UHTTP::getBrowserMSIE()
{
   U_TRACE(0, "UHTTP::getBrowserMSIE()")

   const char* browserMSIE = "";

   // check User-Agent: Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1)

   const char* ptr = getHTTPHeaderValuePtr(*USocket::str_user_agent);

   if (ptr && (u_find(ptr, 30, U_CONSTANT_TO_PARAM("deflate")) != 0)) browserMSIE = "1";

   U_INTERNAL_DUMP("browserMSIE = %S", browserMSIE)

   U_RETURN_POINTER(browserMSIE,const char);
}

// check for "Accept-Encoding: deflate" in header request...

bool UHTTP::isHTTPAcceptEncodingDeflate(uint32_t content_length)
{
   U_TRACE(0, "UHTTP::isHTTPAcceptEncodingDeflate(%u)", content_length)

#ifdef HAVE_LIBZ
   if (content_length > 1400) // SIZE THRESHOLD FOR DEFLATE...
      {
      const char* ptr = getHTTPHeaderValuePtr(*USocket::str_accept_encoding);

      if (ptr && (u_find(ptr, 30, U_CONSTANT_TO_PARAM("deflate")) != 0)) U_RETURN(true);
      }
#endif

   U_RETURN(false);
}

// check for "Connection: close" in headers...

int UHTTP::checkForHTTPConnectionClose()
{
   U_TRACE(0, "UHTTP::checkForHTTPConnectionClose()")

   U_INTERNAL_ASSERT(isHTTPRequest())

   U_INTERNAL_DUMP("http_info.is_connection_close = %d", http_info.is_connection_close);

// U_INTERNAL_ASSERT_DIFFERS(http_info.is_connection_close, U_MAYBE)

   if (http_info.is_connection_close == U_YES)
      {
      (void) UClientImage_Base::pClientImage->handlerWrite();

      U_RETURN(U_PLUGIN_HANDLER_ERROR);
      }

   U_RETURN(U_PLUGIN_HANDLER_FINISHED);
}

void UHTTP::getHTTPInfo(const UString& request, UString& method, UString& uri)
{
   U_TRACE(0, "UHTTP::getHTTPInfo(%.*S,%p,%p)", U_STRING_TO_TRACE(request), &method, &uri)

   U_INTERNAL_DUMP("http_info.uri_len = %u", http_info.uri_len)

   if (http_info.uri_len == 0)
      {
      if (scanfHTTPHeader(request.data()) == false)
         {
         method.clear();
            uri.clear();

         return;
         }

      http_info.startHeader += u_line_terminator_len;
      }

   (void) method.assign(U_HTTP_METHOD_TO_PARAM);
   (void)    uri.assign(U_HTTP_URI_QUERY_TO_PARAM);
}

// param: "[ data expire path domain secure HttpOnly ]"
// ----------------------------------------------------------------------------------------------------------------------------
// string -- Data to put into the cookie        -- must
// int    -- Lifetime of the cookie in HOURS    -- must (0 -> valid until browser exit)
// string -- Path where the cookie can be used  --  opt
// string -- Domain which can read the cookie   --  opt
// bool   -- Secure mode                        --  opt
// bool   -- Only allow HTTP usage              --  opt
// ----------------------------------------------------------------------------------------------------------------------------
// RET: Set-Cookie: ulib_sid=data&expire&HMAC-MD5(data&expire); expires=expire(GMT); path=path; domain=domain; secure; HttpOnly

UString UHTTP::setHTTPCookie(const UString& param)
{
   U_TRACE(0, "UHTTP::setHTTPCookie(%.*S)", U_STRING_TO_TRACE(param))

   /*
   Set-Cookie: NAME=VALUE; expires=DATE; path=PATH; domain=DOMAIN_NAME; secure

   NAME=VALUE
   ------------------------------------------------------------------------------------------------------------------------------------
   This string is a sequence of characters excluding semi-colon, comma and white space. If there is a need to place such data
   in the name or value, some encoding method such as URL style %XX encoding is recommended, though no encoding is defined or required.
   This is the only required attribute on the Set-Cookie header.
   ------------------------------------------------------------------------------------------------------------------------------------

   expires=DATE
   ------------------------------------------------------------------------------------------------------------------------------------
   The expires attribute specifies a date string that defines the valid life time of that cookie. Once the expiration date has been
   reached, the cookie will no longer be stored or given out.
   The date string is formatted as: Wdy, DD-Mon-YYYY HH:MM:SS GMT
   expires is an optional attribute. If not specified, the cookie will expire when the user's session ends.

   Note: There is a bug in Netscape Navigator version 1.1 and earlier. Only cookies whose path attribute is set explicitly to "/" will
   be properly saved between sessions if they have an expires attribute.
   ------------------------------------------------------------------------------------------------------------------------------------

   domain=DOMAIN_NAME
   ------------------------------------------------------------------------------------------------------------------------------------
   When searching the cookie list for valid cookies, a comparison of the domain attributes of the cookie is made with the Internet
   domain name of the host from which the URL will be fetched. If there is a tail match, then the cookie will go through path matching
   to see if it should be sent. "Tail matching" means that domain attribute is matched against the tail of the fully qualified domain
   name of the host. A domain attribute of "acme.com" would match host names "anvil.acme.com" as well as "shipping.crate.acme.com".

   Only hosts within the specified domain can set a cookie for a domain and domains must have at least two (2) or three (3) periods in
   them to prevent domains of the form: ".com", ".edu", and "va.us". Any domain that fails within one of the seven special top level
   domains listed below only require two periods. Any other domain requires at least three. The seven special top level domains are:
   "COM", "EDU", "NET", "ORG", "GOV", "MIL", and "INT".

   The default value of domain is the host name of the server which generated the cookie response.
   ------------------------------------------------------------------------------------------------------------------------------------

   path=PATH
   ------------------------------------------------------------------------------------------------------------------------------------
   The path attribute is used to specify the subset of URLs in a domain for which the cookie is valid. If a cookie has already passed
   domain matching, then the pathname component of the URL is compared with the path attribute, and if there is a match, the cookie is
   considered valid and is sent along with the URL request. The path "/foo" would match "/foobar" and "/foo/bar.html". The path "/" is
   the most general path.

   If the path is not specified, it as assumed to be the same path as the document being described by the header which contains the
   cookie.

   secure
   ------------------------------------------------------------------------------------------------------------------------------------
   If a cookie is marked secure, it will only be transmitted if the communications channel with the host is a secure one. Currently
   this means that secure cookies will only be sent to HTTPS (HTTP over SSL) servers.

   If secure is not specified, a cookie is considered safe to be sent in the clear over unsecured channels. 
   ------------------------------------------------------------------------------------------------------------------------------------

   HttpOnly cookies are a Microsoft extension to the cookie standard. The idea is that cookies marked as httpOnly cannot be accessed
   from JavaScript. This was implemented to stop cookie stealing through XSS vulnerabilities. This is unlike many people believe not
   a way to stop XSS vulnerabilities, but a way to stop one of the possible attacks (cookie stealing) that are possible through XSS.
   */

   UString set_cookie(100U);
   UVector<UString> vec(param);

   if (vec.empty()) set_cookie.snprintf("Set-Cookie: ulib_sid=deleted; expires=%#7D GMT\r\n", u_now.tv_sec - U_ONE_DAY_IN_SECOND);
   else
      {
      U_ASSERT_RANGE(2, vec.size(), 6)

      long num_hours = vec[1].strtol();
      time_t expire  = (num_hours ? u_now.tv_sec + (num_hours * 60 * 60) : 0);

      // HMAC-MD5(data&expire)

      UString data = vec[0], token = UServices::generateToken(data, expire);

      set_cookie.snprintf("Set-Cookie: ulib_sid=%.*s", U_STRING_TO_TRACE(token));

      if (num_hours) set_cookie.snprintf_add("; expires=%#7D GMT", expire);

      UString item;

      for (uint32_t i = 2, n = vec.size(); i < n; ++i)
         {
         item = vec[i];

         if (item.empty() == false)
            {
            switch (i)
               {
               case 2: set_cookie.snprintf_add("; path=%.*s",   U_STRING_TO_TRACE(item)); break; // path
               case 3: set_cookie.snprintf_add("; domain=%.*s", U_STRING_TO_TRACE(item)); break; // domain
               case 4: (void) set_cookie.append(U_CONSTANT_TO_PARAM("; secure"));         break; // secure
               case 5: (void) set_cookie.append(U_CONSTANT_TO_PARAM("; HttpOnly"));       break; // HttpOnly
               }
            }
         }

      (void) set_cookie.append(U_CONSTANT_TO_PARAM("\r\n"));

      U_INTERNAL_DUMP("set_cookie = %.*S", U_STRING_TO_TRACE(set_cookie))
      }

   U_RETURN_STRING(set_cookie);
}

UString UHTTP::getHTTPCookie(bool sh_script)
{
   U_TRACE(1, "UHTTP::getHTTPCookie(%b)", sh_script)

   const char* cookie_ptr = getHTTPHeaderValuePtr(*USocket::str_cookie);

   if (cookie_ptr)
      {
      uint32_t cookie_len = 0;

      for (char c = u_line_terminator[0]; cookie_ptr[cookie_len] != c; ++cookie_len) {}

      U_INTERNAL_DUMP("cookie = %.*S", cookie_len, cookie_ptr)

      if (U_STRNCMP(cookie_ptr, "ulib_sid="))
         {
         if (sh_script) U_RETURN_STRING(UString::getStringNull());

         UString result(cookie_ptr, cookie_len);

         U_RETURN_STRING(result);
         }

      const char* token = cookie_ptr + U_CONSTANT_SIZE("ulib_sid=");

      UString data = UServices::getTokenData(token);

      U_RETURN_STRING(data);
      }

   U_RETURN_STRING(UString::getStringNull());
}

const char* UHTTP::getHTTPStatusDescription()
{
   U_TRACE(0, "UHTTP::getHTTPStatusDescription()")

   const char* descr;

   switch (http_info.nResponseCode)
      {
      // 1xx indicates an informational message only
      case HTTP_CONTINUE:           descr = "Continue";                        break;
      case HTTP_SWITCH_PROT:        descr = "Switching Protocols";             break;

      // 2xx indicates success of some kind
      case HTTP_OK:                 descr = "OK";                              break;
      case HTTP_CREATED:            descr = "Created";                         break;
      case HTTP_ACCEPTED:           descr = "Accepted";                        break;
      case HTTP_NOT_AUTHORITATIVE:  descr = "Non-Authoritative Information";   break;
      case HTTP_NO_CONTENT:         descr = "No Content";                      break;
      case HTTP_RESET:              descr = "Reset Content";                   break;
      case HTTP_PARTIAL:            descr = "Partial Content";                 break;

      // 3xx redirects the client to another URL
      case HTTP_MULT_CHOICE:        descr = "Multiple Choices";                break;
      case HTTP_MOVED_PERM:         descr = "Moved Permanently";               break;
      case HTTP_MOVED_TEMP:         descr = "Moved Temporarily";               break;
      case HTTP_SEE_OTHER:          descr = "See Other";                       break;
      case HTTP_NOT_MODIFIED:       descr = "Not Modified";                    break;
      case HTTP_USE_PROXY:          descr = "Use Proxy";                       break;

      // 4xx indicates an error on the client's part
      case HTTP_BAD_REQUEST:        descr = "Bad Request";                     break;
      case HTTP_UNAUTHORIZED:       descr = "Unauthorized";                    break;
      case HTTP_PAYMENT_REQUIRED:   descr = "Payment Required";                break;
      case HTTP_FORBIDDEN:          descr = "Forbidden";                       break;
      case HTTP_NOT_FOUND:          descr = "Not Found";                       break;
      case HTTP_BAD_METHOD:         descr = "Method Not Allowed";              break;
      case HTTP_NOT_ACCEPTABLE:     descr = "Not Acceptable";                  break;
      case HTTP_PROXY_AUTH:         descr = "Proxy Authentication Required";   break;
      case HTTP_CLIENT_TIMEOUT:     descr = "Request Timeout";                 break;
      case HTTP_CONFLICT:           descr = "Conflict";                        break;
      case HTTP_GONE:               descr = "Gone";                            break;
      case HTTP_LENGTH_REQUIRED:    descr = "Length Required";                 break;
      case HTTP_PRECON_FAILED:      descr = "Precondition Failed";             break;
      case HTTP_ENTITY_TOO_LARGE:   descr = "Request Entity Too Large";        break;
      case HTTP_REQ_TOO_LONG:       descr = "Request-URI Too Long";            break;
      case HTTP_UNSUPPORTED_TYPE:   descr = "Unsupported Media Type";          break;
      case HTTP_REQ_RANGE_NOT_OK:   descr = "Requested Range not satisfiable"; break;
      case HTTP_EXPECTATION_FAILED: descr = "Expectation Failed";              break;

      // 5xx indicates an error on the server's part
      case HTTP_INTERNAL_ERROR:     descr = "Internal Server Error";           break;
      case HTTP_NOT_IMPLEMENTED:    descr = "Not Implemented";                 break;
      case HTTP_BAD_GATEWAY:        descr = "Bad Gateway";                     break;
      case HTTP_UNAVAILABLE:        descr = "Service Unavailable";             break;
      case HTTP_GATEWAY_TIMEOUT:    descr = "Gateway Timeout";                 break;
      case HTTP_VERSION:            descr = "HTTP Version Not Supported";      break;

      default:                      descr = "Code unknown";                    break;
      }

   U_RETURN(descr);
}

const char* UHTTP::getHTTPStatus()
{
   U_TRACE(0, "UHTTP::getHTTPStatus()")

   static char buffer[128];

   (void) sprintf(buffer, "(%d, %s)", http_info.nResponseCode, getHTTPStatusDescription());

   U_RETURN(buffer);
}

void UHTTP::getTimeIfNeeded()
{
   U_TRACE(0, "UHTTP::getTimeIfNeeded()")

   if (http_info.version == 0 &&
       (UServer_Base::isLog() == false || ULog::isSysLog()))
      {
      (void) U_SYSCALL(gettimeofday, "%p,%p", &u_now, 0);
      }
}

UString UHTTP::getHTMLDirectoryList()
{
   U_TRACE(0, "UHTTP::getHTMLDirectoryList()")

   UString buffer(U_CAPACITY);

   buffer.snprintf("<html><head><title></title></head>"
                   "<body><h1>Index of directory: %.*s</h1><hr>"
                     "<table><tr>"
                        "<td><a href=\"/%.*s/..\"><img align=\"absbottom\" border=\"0\" src=\"/icons/menu.gif\"> Up one level</a></td>"
                        "<td></td>"
                        "<td></td>"
                     "</tr>", U_FILE_TO_TRACE(*file), U_FILE_TO_TRACE(*file));

   if (UFile::chdir(file->getPathRelativ(), true))
      {
      UFile item;
      bool is_dir;
      UVector<UString> vec;
      UString entry(U_CAPACITY);
      uint32_t pos = buffer.size(), n = UFile::listContentOf(vec);

      if (n > 1) vec.sort();

      for (uint32_t i = 0; i < n; ++i)
         {
         item.setPath(vec[i]);

         if (item.stat())
            {
            is_dir = item.dir();

            entry.snprintf("<tr>"
                              "<td><a href=\"/%.*s/%.*s\"><img align=\"absbottom\" border=\"0\" src=\"/icons/%s.gif\"> %.*s</a></td>"
                              "<td align=\"right\" valign=\"bottom\">%u KB</td>"
                              "<td align=\"right\" valign=\"bottom\">%#4D</td>"
                            "</tr>",
                            U_FILE_TO_TRACE(*file), U_FILE_TO_TRACE(item), (is_dir ? "menu" : "gopher-unknown"), U_FILE_TO_TRACE(item),
                            item.getSize() / 1024,
                            item.st_mtime);

            if (is_dir)
               {
               (void) buffer.insert(pos, entry);

               pos += entry.size();
               }
            else
               {
               (void) buffer.append(entry);
               }
            }
         }

      (void) UFile::chdir(0, true);

      (void) buffer.append(U_CONSTANT_TO_PARAM("</table><hr><address>ULib Server</address>\r\n</body></html>"));
      }

   U_INTERNAL_DUMP("buffer(%u) = %#.*S", buffer.size(), U_STRING_TO_TRACE(buffer));

   U_RETURN_STRING(buffer);
}

void UHTTP::resetForm()
{
   U_TRACE(0, "UHTTP::resetForm()")

   U_INTERNAL_ASSERT_POINTER(qcontent)
   U_INTERNAL_ASSERT_POINTER(formMulti)
   U_INTERNAL_ASSERT_POINTER(form_name_value)

         formMulti->clear();
   form_name_value->clear();
          qcontent->clear();

   // clean temporary directory, if any...

   if (tmpdir->empty() == false)
      {
      (void) UFile::rmdir(tmpdir->data(), true);

      tmpdir->setEmpty();
      }
}

// retrieve information on specific HTML form elements
// (such as checkboxes, radio buttons, and text fields), on uploaded files

void UHTTP::getFormValue(UString& buffer, uint32_t n)
{
   U_TRACE(0, "UHTTP::getFormValue(%.*S,%u)", U_STRING_TO_TRACE(buffer), n)

   U_ASSERT_MAJOR(buffer.capacity(),1000)
   U_INTERNAL_ASSERT_POINTER(form_name_value)

   if (n >= form_name_value->size())
      {
      buffer.setEmpty();

      return;
      }

   (void) buffer.replace((*form_name_value)[n]);
}

uint32_t UHTTP::processHTTPForm()
{
   U_TRACE(0, "UHTTP::processHTTPForm()")

   uint32_t n = 0;
   const char* ptr;

   if (isHttpGET())
      {
      *qcontent = UString(U_HTTP_QUERY_TO_PARAM);

      goto end;
      }

   // ------------------------------------------------------------------------
   // POST
   // ------------------------------------------------------------------------
   // Content-Type: application/x-www-form-urlencoded OR multipart/form-data...
   // ------------------------------------------------------------------------

   U_ASSERT(isHttpPOST())

   ptr = getHTTPHeaderValuePtr(*USocket::str_content_type);

   if (ptr == 0) U_RETURN(0);

   if (U_STRNCMP(ptr, "application/x-www-form-urlencoded") == 0)
      {
      *qcontent = *UClientImage_Base::body;

      goto end;
      }

   // multipart/form-data (FILE UPLOAD)

   U_INTERNAL_ASSERT_EQUALS(U_STRNCMP(ptr, "multipart/form-data"), 0)

   {
   UString boundary;
   UTokenizer(UClientImage_Base::body->substr(2), u_line_terminator).next(boundary, (bool*)0);

   formMulti->setBoundary(boundary);
   formMulti->setContent(*UClientImage_Base::body);

   if (formMulti->parse() == false) U_RETURN(0);
   }

   // create temporary directory with files upload...

   {
   tmpdir->snprintf("%s/formXXXXXX", u_tmpdir);

   if (UFile::mkdtemp(*tmpdir) == false) U_RETURN(0);

   UMimeEntity* item;
   UString content, name, filename, basename, pathname(100U);

   for (uint32_t i = 0, j = formMulti->getNumBodyPart(); i < j; ++i)
      {
      item    = (*formMulti)[i];
      content = item->getContent();

      // Content-Disposition: form-data; name="input_file"; filename="/tmp/4dcd39e8-2a84-4242-b7bc-ca74922d26e1"

      if (UMimeHeader::getNames(item->getContentDisposition(), name, filename))
         {
         // NB: we can't reuse the same string (filename) to avoid DEAD OF SOURCE STRING WITH CHILD ALIVE...

         basename = UStringExt::basename(filename);

         pathname.snprintf("%.*s/%.*s", U_STRING_TO_TRACE(*tmpdir), U_STRING_TO_TRACE(basename));

         (void) UFile::writeTo(pathname, content);

         content = pathname;
         }

      form_name_value->push_back(name);
      form_name_value->push_back(content);
      }

   n = form_name_value->size();

   U_RETURN(n);
   }

end:
   if (qcontent->empty() == false) n = UStringExt::getNameValueFromData(*qcontent, *form_name_value);

   U_RETURN(n);
}

// set HTTP main error message
// --------------------------------------------------------------------------------------------------------------------------------------

U_NO_EXPORT UString UHTTP::getHTTPHeaderForResponse()
{
   U_TRACE(0, "UHTTP::getHTTPHeaderForResponse()")

   /*
   Max Keep-Alive Requests

   Description: Specifies the maximum number of requests that can be served through a Keep-Alive (Persistent) session.
                Connection will be closed once this limit is reached.

   Syntax: Integer number

   Tips: [Performance] Set it to a resonable high value (256). Value <= 1 will disable Keep-Alive.
   */

   U_INTERNAL_DUMP("http_info.version = %u http_info.keep_alive = %u http_info.is_connection_close = %u",
                    http_info.version,     http_info.keep_alive,     http_info.is_connection_close)

   if (UServer_Base::isMaxKeepAlive())
      {
      http_info.keep_alive          = 0;
      http_info.is_connection_close = U_YES;
      }

   // HTTP/1.1 compliance: Sends Date on every requests...

   UString connection(200U);

   if (http_info.version) connection.snprintf("Date: %D\r\n", 0);
   else
      {
      // HTTP/1.0 compliance: if not Keep-Alive we force close...

      if (http_info.keep_alive == 0) http_info.is_connection_close = U_YES;
      else
         {
         // ...to indicate that it desires a multiple-request session

         (void) connection.append(U_CONSTANT_TO_PARAM("Connection: keep-alive\r\n"));

         /*
         Keep-Alive Timeout

         Description: Specifies the maximum idle time between requests from a Keep-Alive connection. If no new request is received during
                      this period of time, the connection will be closed.

         Syntax:      Integer number

         Tips: [Security & Performance] We recommend you to set the value just long enough to handle all requests for a single page view.
               It is unnecessary to keep connection alive for an extended period of time. A smaller value can reduce idle connections, increase
               capacity to service more users and guard against DoS attacks. 2-5 seconds is a reasonable range for most applications.
         */

         if (UServer_Base::getReqTimeout())
            {
            // ...to indicate that the session is being kept alive for a maximum of x requests and a per-request timeout of x seconds

            connection.snprintf_add("Keep-Alive: max=%d, timeout=%d\r\n", UServer_Base::getMaxKeepAlive(), UServer_Base::getReqTimeout());
            }
         }
      }

   if (http_info.is_connection_close == U_YES) (void) connection.append(U_CONSTANT_TO_PARAM("Connection: close\r\n"));

   U_RETURN_STRING(connection);
}

U_NO_EXPORT UString UHTTP::getHTTPHeaderForResponse(int nResponseCode, UString& content)
{
   U_TRACE(0, "UHTTP::getHTTPHeaderForResponse(%d,%.*S)", nResponseCode, U_STRING_TO_TRACE(content))

   U_INTERNAL_ASSERT_MAJOR(nResponseCode,0)

   if ((http_info.nResponseCode = nResponseCode) == HTTP_NOT_IMPLEMENTED)
      {
      U_ASSERT(content.empty())

      (void) content.assign(U_CONSTANT_TO_PARAM("Allow: GET, HEAD, POST\r\nContent-Length: 0\r\n\r\n"));
      }

   // check for empty content....

   uint32_t sz     = content.size();
   const char* ptr = content.data();

   if (sz == 0)
      {
      ptr =                 "Content-Length: 0\r\n\r\n";
      sz  = U_CONSTANT_SIZE("Content-Length: 0\r\n\r\n");
      }

   UString tmp(300U + content.size()), connection = getHTTPHeaderForResponse();

   tmp.snprintf(str_frm_response->data(),
                http_info.version + '0',
                nResponseCode, getHTTPStatusDescription(),
                U_STRING_TO_TRACE(connection),
                sz, ptr);

   U_INTERNAL_DUMP("tmp(%u) = %#.*S", tmp.size(), U_STRING_TO_TRACE(tmp));

   U_RETURN_STRING(tmp);
}

UString UHTTP::getHTTPHeaderForResponse(int nResponseCode, const char* content_type, const UString& body)
{
   U_TRACE(0, "UHTTP::getHTTPHeaderForResponse(%d,%S,%.*S)", nResponseCode, content_type, U_STRING_TO_TRACE(body))

   U_INTERNAL_ASSERT_MAJOR(nResponseCode,0)

   UString tmp(300U);

   if (content_type)
      {
      U_ASSERT_EQUALS(body.empty(),false)

      tmp.snprintf("Content-Length: %u\r\n"
                   "Content-Type: %s\r\n"
                   "\r\n", body.size(), content_type);
      }

   tmp = getHTTPHeaderForResponse(nResponseCode, tmp);

   if (content_type) (void) tmp.append(body);

   U_INTERNAL_DUMP("tmp(%u) = %#.*S", tmp.size(), U_STRING_TO_TRACE(tmp));

   U_RETURN_STRING(tmp);
}

UString UHTTP::getHTTPRedirectResponse(const UString& ext, const char* ptr_location, uint32_t len_location)
{
   U_TRACE(0, "UHTTP::getHTTPRedirectResponse(%.*S,%.*S,%u)", U_STRING_TO_TRACE(ext), len_location, ptr_location, len_location)

   U_ASSERT_EQUALS(u_find(ptr_location,len_location,"\n",1),0)

   if (http_info.is_connection_close == U_MAYBE) http_info.is_connection_close = U_YES;

   UString connection = getHTTPHeaderForResponse() + ext,
           tmp(800U + connection.size() + str_frm_moved_temp->size() + len_location);

   tmp.snprintf(str_frm_moved_temp->data(),
                http_info.version + '0',
                U_STRING_TO_TRACE(connection),
                len_location, ptr_location,
                237 + len_location,
                len_location, ptr_location);

   U_INTERNAL_DUMP("tmp(%u) = %#.*S", tmp.size(), U_STRING_TO_TRACE(tmp));

   U_RETURN_STRING(tmp);
}

void UHTTP::setHTTPForbidden()
{
   U_TRACE(0, "UHTTP::setHTTPForbidden()")

   http_info.is_connection_close = U_YES;

   UClientImage_Base::wbuffer->setBuffer(300U + str_frm_forbidden->size() + http_info.uri_len);
   UClientImage_Base::wbuffer->snprintf(str_frm_forbidden->data(), http_info.version + '0', 254 + http_info.uri_len, U_HTTP_URI_TO_TRACE);
}

void UHTTP::setHTTPNotFound()
{
   U_TRACE(0, "UHTTP::setHTTPNotFound()")

   http_info.is_connection_close = U_YES;

   UClientImage_Base::wbuffer->setBuffer(300U + str_frm_not_found->size() + http_info.uri_len);
   UClientImage_Base::wbuffer->snprintf(str_frm_not_found->data(), http_info.version + '0', 250 + http_info.uri_len, U_HTTP_URI_TO_TRACE);
}

void UHTTP::setHTTPUnAuthorized(bool digest)
{
   U_TRACE(0, "UHTTP::setHTTPUnAuthorized(%b)", digest)

#ifndef HAVE_SSL
   setHTTPForbidden();
#else
   UString buffer(100U);
   http_info.is_connection_close = U_YES;

                        (void) buffer.assign(U_CONSTANT_TO_PARAM("WWW-Authenticate: "));
   if (digest == false) (void) buffer.append(U_CONSTANT_TO_PARAM("Basic"));
   else
      {
      buffer.snprintf_add("Digest qop=\"auth\", nonce=\"%ld\", algorithm=MD5,", u_now.tv_sec);
      }

   (void) buffer.append(U_CONSTANT_TO_PARAM(" realm=\"" U_HTTP_REALM "\"\r\n\r\n"));

   *UClientImage_Base::wbuffer = getHTTPHeaderForResponse(HTTP_UNAUTHORIZED, buffer);
#endif
}

void UHTTP::setHTTPBadRequest()
{
   U_TRACE(0, "UHTTP::setHTTPBadRequest()")

   http_info.is_connection_close = U_YES;

   *UClientImage_Base::wbuffer = getHTTPHeaderForResponse(HTTP_BAD_REQUEST, U_CTYPE_HTML, *str_frm_bad_request);
}

void UHTTP::setHTTPInternalError()
{
   U_TRACE(0, "UHTTP::setHTTPInternalError()")

   http_info.is_connection_close = U_YES;

   *UClientImage_Base::wbuffer = getHTTPHeaderForResponse(HTTP_INTERNAL_ERROR, U_CTYPE_HTML, *str_frm_internal_error);
}

void UHTTP::setHTTPServiceUnavailable()
{
   U_TRACE(0, "UHTTP::setHTTPServiceUnavailable()")

   http_info.is_connection_close = U_YES;

   *UClientImage_Base::wbuffer = getHTTPHeaderForResponse(HTTP_UNAVAILABLE, U_CTYPE_HTML, *str_frm_service_unavailable);
}

void UHTTP::setHTTPCgiResponse(int nResponseCode, uint32_t content_length, bool header_content_length)
{
   U_TRACE(0, "UHTTP::setHTTPCgiResponse(%d,%u,%b)", nResponseCode, content_length, header_content_length)

   U_ASSERT_EQUALS(UClientImage_Base::wbuffer->empty(), false)

   UString tmp(800U);

   if (header_content_length == false &&
              content_length)
      {
      if (isHTTPAcceptEncodingDeflate(content_length) == false) tmp.snprintf("Content-Length: %u\r\n", content_length);
      else
         {
         uint32_t endHeader = UClientImage_Base::wbuffer->size() - content_length;
         UString content    = UClientImage_Base::wbuffer->substr(endHeader),
                 compress   = UStringExt::deflate(content);

         content_length = compress.size();

         tmp.snprintf("Content-Length: %u\r\n"
                      "Content-Encoding: deflate\r\n"
                      "%.*s",
                      content_length,
                      endHeader, UClientImage_Base::wbuffer->data());

#     ifdef DEBUG
         content.clear(); // NB: to avoid DEAD OF SOURCE STRING WITH CHILD ALIVE...
#     endif

         *UClientImage_Base::wbuffer = compress;
         }
      }

   UClientImage_Base::wbuffer->insert(0, getHTTPHeaderForResponse(nResponseCode, tmp));
}

// --------------------------------------------------------------------------------------------------------------------------------------

U_NO_EXPORT bool UHTTP::openFile()
{
   U_TRACE(0, "UHTTP::openFile()")

   bool result;

   if (file->dir())
      {
      // NB: cgi-bin and usp are forbidden...

      result = (u_endsWith(U_FILE_TO_PARAM(*file), U_CONSTANT_TO_PARAM("usp"))     == false &&
                u_endsWith(U_FILE_TO_PARAM(*file), U_CONSTANT_TO_PARAM("cgi-bin")) == false);

      if (result)
         {
         // Check for an index file... (index.html)

         UString buffer(U_CAPACITY), old = file->getPath();

         buffer.size_adjust(UFile::setPathFromFile(*file, buffer.data(), U_CONSTANT_TO_PARAM("/index.html")));

         if (file->open(buffer)) file->fstat();
         else
            {
            file->setPath(old);

            /* Nope, no index file, so it's an actual directory request */

            result = (file->getPathRelativLen() > 1); // NB: '/' alias '.' is forbidden...
            }
         }
      }
   else
      {
      result = (file->regular()                                           &&
                file->isPathRelativ(*UServer_Base::str_htpasswd) == false && // NB: '.htpasswd' is forbidden
                file->isPathRelativ(*UServer_Base::str_htdigest) == false && // NB: '.htdigest' is forbidden
                file->open());
      }

   U_RETURN(result);
}

bool UHTTP::processHTTPAuthorization(bool digest, const UString& request_uri)
{
   U_TRACE(0, "UHTTP::processHTTPAuthorization(%b,%.*S)", digest, U_STRING_TO_TRACE(request_uri))

   bool result = false;

#ifdef HAVE_SSL
   const char* ptr = getHTTPHeaderValuePtr(*USocket::str_authorization);

   if (ptr == 0) U_RETURN(false);

   if (digest)
      {
      if (U_STRNCMP(ptr, "Digest ")) U_RETURN(false);

      ptr += U_CONSTANT_SIZE("Digest ");
      }
   else
      {
      if (U_STRNCMP(ptr, "Basic "))  U_RETURN(false);

      ptr += U_CONSTANT_SIZE("Basic ");
      }

   UString content;
   UTokenizer t(UClientImage_Base::rbuffer->substr(UClientImage_Base::rbuffer->distance(ptr)), u_line_terminator);

   if (t.next(content, (bool*)0) == false) U_RETURN(false);

   UString user(100U);

   if (digest)
      {
      // Authorization: Digest username="s.casazza", realm="Protected Area", nonce="dcd98b7102dd2f0e8b11d0f600bfb0c093", uri="/",
      //                       response="a74c1cb52877766bb0781d12b653d1a7", qop=auth, nc=00000001, cnonce="73ed2d9694b46324", algorithm=MD5

      UVector<UString> name_value;
      UString name, value, realm, nonce, uri, response, qop, nc, cnonce;
      uint32_t i = 0, n = UStringExt::getNameValueFromData(content, name_value, U_CONSTANT_TO_PARAM(", \t"));

      // gcc: cannot optimize loop, the loop counter may overflow ???

      while (i < n)
         {
         name  = name_value[i++];
         value = name_value[i++];

         U_INTERNAL_DUMP("name = %.*S value = %.*S", U_STRING_TO_TRACE(name), U_STRING_TO_TRACE(value))

         switch (name.c_char(0))
            {
            case 'u':
               {
               if (name.equal(U_CONSTANT_TO_PARAM("username")))
                  {
                  U_ASSERT(user.empty())

                  user = value;
                  }
               else if (name.equal(U_CONSTANT_TO_PARAM("uri")))
                  {
                  U_ASSERT(uri.empty())

                  // NB: may be a ALIAS...

                  if (value != request_uri) U_RETURN(false);

                  uri = value;
                  }
               }
            break;

            case 'r':
               {
               if (name.equal(U_CONSTANT_TO_PARAM("realm")))
                  {
                  U_ASSERT(realm.empty())

                  realm = value;
                  }
               else if (name.equal(U_CONSTANT_TO_PARAM("response")))
                  {
                  U_ASSERT(response.empty())

                  if (value.size() != 32) U_RETURN(false);

                  response = value;
                  }
               }
            break;

            case 'n':
               {
               if (name.equal(U_CONSTANT_TO_PARAM("nonce")))
                  {
                  U_ASSERT(nonce.empty())

                  // XXX: Due to a bug in MSIE (version=??), we do not check for authentication timeout...

                  if ((u_now.tv_sec - value.strtol()) > 3600) U_RETURN(false);

                  nonce = value;
                  }
               else if (name.equal(U_CONSTANT_TO_PARAM("nc")))
                  {
                  U_ASSERT(nc.empty())

                  nc = value;
                  }
               }
            break;

            case 'q':
               {
               if (name.equal(U_CONSTANT_TO_PARAM("qop")))
                  {
                  U_ASSERT(qop.empty())

                  qop = value;
                  }
               }
            break;

            case 'c':
               {
               if (name.equal(U_CONSTANT_TO_PARAM("cnonce")))
                  {
                  U_ASSERT(cnonce.empty())

                  cnonce = value;
                  }
               }
            break;

            case 'a':
               {
               if (name.equal(U_CONSTANT_TO_PARAM("algorithm")))
                  {
                  if (value.equal("MD5") == false) U_RETURN(false);
                  }
               }
            break;
            }
         }

      UString  a2(4 + 1 + uri.size()),                     //     method : uri
              ha2(33U),                                    // MD5(method : uri)
              ha1 = UServer_Base::getUserHA1(user, realm), // MD5(user : realm : password)
              a3(200U), ha3(33U);

      // MD5(method : uri)

      a2.snprintf("%.*s:%.*s", U_HTTP_METHOD_TO_TRACE, U_STRING_TO_TRACE(uri));

      UServices::generateDigest(U_HASH_MD5, 0, a2, ha2, false);

      // MD5(HA1 : nonce : nc : cnonce : qop : HA2)

      a3.snprintf("%.*s:%.*s:%.*s:%.*s:%.*s:%.*s", U_STRING_TO_TRACE(ha1), U_STRING_TO_TRACE(nonce),
                                                   U_STRING_TO_TRACE(nc),  U_STRING_TO_TRACE(cnonce),
                                                   U_STRING_TO_TRACE(qop), U_STRING_TO_TRACE(ha2));

      UServices::generateDigest(U_HASH_MD5, 0, a3, ha3, false);

      result = (ha3 == response);
      }
   else
      {
      // Authorization: Basic cy5jYXNhenphOnN0ZWZhbm8x==

      UString buffer(100U);

      if (UBase64::decode(content, buffer))
         {
         U_INTERNAL_DUMP("buffer = %.*S", U_STRING_TO_TRACE(buffer))

         t.setData(buffer);
         t.setDelimiter(":");

         UString password(100U);

         if (t.next(user,     (bool*)0) &&
             t.next(password, (bool*)0) &&
             UServer_Base::isUserAuthorized(user, password))
            {
            result = true;
            }
         }
      }

   U_SRV_LOG_VAR("request authorization for user '%.*s' %s", U_STRING_TO_TRACE(user), result ? "success" : "failed");
#endif

   U_RETURN(result);
}

bool UHTTP::checkHTTPRequest()
{
   U_TRACE(0, "UHTTP::checkHTTPRequest()")

   U_INTERNAL_DUMP("method = %.*S uri = %.*S", U_HTTP_METHOD_TO_TRACE, U_HTTP_URI_TO_TRACE)

   U_INTERNAL_ASSERT(isHTTPRequest())

   // ...process the HTTP message

   UString pathname(U_CAPACITY);

   pathname.snprintf("%w%.*s", U_HTTP_URI_TO_TRACE);

   u_canonicalize_pathname(pathname.data());

   pathname.size_adjust();

   if (UServer_Base::isFileInsideDocumentRoot(pathname) == false) // like chroot()...
      {
      // set forbidden error response...

      setHTTPForbidden();

      U_RETURN(false);
      }

   file->setPath(pathname);

   if (file->stat() == false)
      {
      // set not found error response...

      setHTTPNotFound();

      U_RETURN(false);
      }

   U_RETURN(true);
}

// manage CGI

bool UHTTP::isPHPRequest()
{
   U_TRACE(0, "UHTTP::isPHPRequest()")

   U_INTERNAL_ASSERT(isHTTPRequest())

   if (http_info.is_php == U_MAYBE) http_info.is_php = (u_endsWith(U_HTTP_URI_TO_PARAM, U_CONSTANT_TO_PARAM(".php")) ? U_YES : U_NOT);

   bool result = (http_info.is_php == U_YES);

   U_RETURN(result);
}

bool UHTTP::isCGIRequest()
{
   U_TRACE(0, "UHTTP::isCGIRequest()")

   const char* ptr = U_HTTP_URI + http_info.uri_len - 1;

   U_INTERNAL_ASSERT_POINTER(ptr)

   if    (*ptr == '/') U_RETURN(false); // NB: ends with '/'...
   while (*ptr != '/') --ptr;
   while (*ptr == '/') --ptr;

   uint32_t sz = U_CONSTANT_SIZE("/cgi-bin/");

   ptr -= sz - 2;

   U_INTERNAL_DUMP("ptr = %.*S", sz, ptr)

   bool result = (memcmp(ptr, "/cgi-bin/", sz) == 0);

   U_RETURN(result);
}

void UHTTP::setCGIEnvironment(UString& environment, bool sh_script)
{
   U_TRACE(0, "UHTTP::setCGIEnvironment(%.*S,%b)", U_STRING_TO_TRACE(environment), sh_script)

   // Referer: http://www.cgi101.com/class/ch3/text.html

   uint32_t    referer_len = 0;
   const char* referer_ptr = getHTTPHeaderValuePtr(*USocket::str_referer);

   if (referer_ptr)
      {
      for (char c = u_line_terminator[0]; referer_ptr[referer_len] != c; ++referer_len) {}

      U_INTERNAL_DUMP("referer = %.*S", referer_len, referer_ptr)
      }

   // Cookie: _saml_idp=dXJuOm1hY2U6dGVzdC5zXRo; _redirect_user_idp=urn%3Amace%3Atest.shib%3Afederation%3Alocalhost;

   UString cookie = getHTTPCookie(sh_script);

   U_INTERNAL_DUMP("cookie = %.*S", U_STRING_TO_TRACE(cookie))

   /*
   Set up CGI environment variables.
   The following table describes common CGI environment variables that the server creates (some of these are not available with some servers):

   CGI server variable  Description
   -------------------------------------------------------------------------------------------------------------------------------------------
   SERVER_SOFTWARE
      Name and version of the information server software answering the request (and running the gateway). Format: name/version.
   SERVER_NAME
      Server's hostname, DNS alias, or IP address as it appears in self-referencing URLs.
   GATEWAY_INTERFACE
      CGI specification revision with which this server complies. Format: CGI/revision.
   SERVER_PROTOCOL
      Name and revision of the information protocol this request came in with. Format: protocol/revision.
   SERVER_PORT
      Port number to which the request was sent.
   REQUEST_METHOD
      Method with which the request was made. For HTTP, this is Get, Head, Post, and so on.
   PATH_INFO
      Extra path information, as given by the client. Scripts can be accessed by their virtual pathname, followed by extra information at the end
      of this path. The extra information is sent as PATH_INFO.
   PATH_TRANSLATED
      Translated version of PATH_INFO after any virtual-to-physical mapping.
   SCRIPT_NAME
      Virtual path to the script that is executing; used for self-referencing URLs.
   QUERY_STRING
      Query information that follows the ? in the URL that referenced this script.
   REMOTE_HOST
      Hostname making the request. If the server does not have this information, it sets REMOTE_ADDR and does not set REMOTE_HOST.
   REMOTE_ADDR
      IP address of the remote host making the request.
   AUTH_TYPE
      If the server supports user authentication, and the script is protected, the protocol-specific authentication method used to validate the user.
   AUTH_USER
   REMOTE_USER
      If the server supports user authentication, and the script is protected, the username the user has authenticated as. (Also available as AUTH_USER.)
   REMOTE_IDENT
      If the HTTP server supports RFC 931 identification, this variable is set to the remote username retrieved from the server.
      Use this variable for logging only.
   CONTENT_TYPE
      For queries that have attached information, such as HTTP POST and PUT, this is the content type of the data.
   CONTENT_LENGTH
      Length of the content as given by the client.

   CERT_ISSUER
      Issuer field of the client certificate (O=MS, OU=IAS, CN=user name, C=USA).
   CERT_SUBJECT
      Subject field of the client certificate.
   CERT_SERIALNUMBER
      Serial number field of the client certificate.
   -------------------------------------------------------------------------------------------------------------------------------------------

   The following table describes common CGI environment variables the browser creates and passes in the request header:

   CGI client variable  Description
   -------------------------------------------------------------------------------------------------------------------------------------------
   HTTP_REFERER
      The referring document that linked to or submitted form data.
   HTTP_USER_AGENT
      The browser that the client is currently using to send the request. Format: software/version library/version.
   HTTP_IF_MODIFIED_SINCE
      The last time the page was modified. The browser determines whether to set this variable, usually in response to the server having sent
      the LAST_MODIFIED HTTP header. It can be used to take advantage of browser-side caching.
   -------------------------------------------------------------------------------------------------------------------------------------------

   Example:
   ----------------------------------------------------------------------------------------------------------------------------
   GATEWAY_INTERFACE=CGI/1.1
   QUERY_STRING=
   REMOTE_ADDR=127.0.0.1
   REQUEST_METHOD=GET
   SCRIPT_NAME=/cgi-bin/printenv
   SERVER_NAME=localhost
   SERVER_PORT=80
   SERVER_PROTOCOL=HTTP/1.1
   SERVER_SOFTWARE=Apache

   HTTP_ACCEPT_LANGUAGE="en-us,en;q=0.5"
   PATH="/lib64/rc/sbin:/lib64/rc/bin:/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin"
   SCRIPT_FILENAME="/var/www/localhost/cgi-bin/printenv"
   HTTP_COOKIE="_saml_idp=dXJuOm1hY2U6dGVzdC5zXRo; _redirect_user_idp=urn%3Amace%3Atest.shib%3Afederation%3Alocalhost;"

   DOCUMENT_ROOT="/var/www/localhost/htdocs"
   HTTP_ACCEPT="text/xml,application/xml,application/xhtml+xml,text/html;q=0.9,text/plain;q=0.8,image/png"
   HTTP_ACCEPT_CHARSET="ISO-8859-1,utf-8;q=0.7,*;q=0.7"
   HTTP_ACCEPT_ENCODING="gzip,deflate"
   HTTP_ACCEPT_LANGUAGE="en-us,en;q=0.5"
   HTTP_CONNECTION="keep-alive"
   HTTP_HOST="localhost"
   HTTP_KEEP_ALIVE="300"
   HTTP_USER_AGENT="Mozilla/5.0 (X11; U; Linux x86_64; en-US; rv:1.8.1.14) Gecko/20080421 Firefox/2.0.0.14"
   REQUEST_URI="/cgi-bin/printenv"
   REMOTE_PORT="35653"
   SERVER_ADDR="127.0.0.1"
   SERVER_ADMIN="root@localhost"
   SERVER_SIGNATURE="<address>Apache Server at localhost Port 80</address>\n"
   UNIQUE_ID="b032zQoeAYMAAA-@BZwAAAAA"
   ----------------------------------------------------------------------------------------------------------------------------

   http://$SERVER_NAME:$SERVER_PORT$SCRIPT_NAME$PATH_INFO will always be an accessible URL that points to the current script...
   */

   uint32_t ip_client_len;
   const char* ip_client = UClientImage_Base::getRemoteInfo(&ip_client_len);
   UString buffer(U_CAPACITY), name = UServer_Base::getNodeName(), ip_server = UServer_Base::getIPAddress();

   buffer.snprintf("\n"
                   "CONTENT_LENGTH=%u\n"
                   "GATEWAY_INTERFACE=CGI/1.1\n"
                   "QUERY_STRING=%.*s\n"
                   "REMOTE_ADDR=%.*s\n"
                   "REQUEST_METHOD=%.*s\n"
                   "SCRIPT_NAME=%.*s\n"
                   "SERVER_NAME=%.*s\n"
                   "SERVER_PORT=%d\n"
                   "SERVER_PROTOCOL=HTTP/1.%c\n"
                   "SERVER_SOFTWARE=" PACKAGE "/" VERSION "\n"
                   // ext
                   "SERVER_ADDR=%.*s\n"
                   "BROWSER_MSIE=%s\n"
                   "HTTP_ACCEPT_LANGUAGE=%.2s\n"
                   "HTTP_COOKIE=%.*s\n"
                   "HTTP_REFERER=%.*s\n"
                   "PATH=/usr/local/bin:/usr/bin:/bin\n"
                   "PWD=%w%.*s\n"
                   "SCRIPT_FILENAME=%w%.*s\n",
                   UClientImage_Base::body->size(),
                   U_HTTP_QUERY_TO_TRACE,
                   ip_client_len, ip_client,
                   U_HTTP_METHOD_TO_TRACE,
                   U_HTTP_URI_TO_TRACE,
                   U_STRING_TO_TRACE(name),
                   UServer_Base::port,
                   http_info.version + '0',
                   U_STRING_TO_TRACE(ip_server),
                   (sh_script ? UHTTP::getBrowserMSIE() : ""),
                   UHTTP::getAcceptLanguage(),
                   U_STRING_TO_TRACE(cookie),
                   referer_len, referer_ptr,
                   U_HTTP_URI_TO_TRACE,
                   U_HTTP_URI_TO_TRACE);

#ifdef HAVE_SSL
   if (UClientImage_Base::socket->isSSL())
      {
      (void) buffer.append(U_CONSTANT_TO_PARAM("HTTPS=1\n"));

      X509* x509 = ((USSLSocket*)UClientImage_Base::socket)->getPeerCertificate();

      if (x509)
         {
         UString issuer  = UCertificate::getIssuer(x509),
                 subject = UCertificate::getSubject(x509);

         buffer.snprintf_add("\"SSL_CLIENT_I_DN=%.*s\"\n"
                             "\"SSL_CLIENT_S_DN=%.*s\"\n"
                             "SSL_CLIENT_CERT_SERIAL=%ld\n",
                             U_STRING_TO_TRACE(issuer),
                             U_STRING_TO_TRACE(subject),
                             UCertificate::getSerialNumber(x509));
         }
      }
#endif

   (void) environment.append(buffer);
}

void UHTTP::setCGIShellScript(UString& command)
{
   U_TRACE(0, "UHTTP::setCGIShellScript(%.*S)", U_STRING_TO_TRACE(command))

   // ULIB facility: check if present form data and convert it in parameters for shell script...

   char c;
   UString item;
   const char* ptr;
   uint32_t sz, n = processHTTPForm();

   U_INTERNAL_ASSERT_POINTER(form_name_value)

   for (uint32_t i = 1; i < n; i += 2)
      {
      item = (*form_name_value)[i];

      // check for binary data (invalid)...

      if (item.empty() ||
          item.isBinary())
         {
         c    = '\'';
         ptr  = 0;
         sz   = 0;

         if (item.empty() == false) U_WARNING("Found binary data in form: %.*s", U_STRING_TO_TRACE(item));
         }
      else
         {
         ptr  = item.data();
         sz   = item.size();

         // find how to escape the param...

         c = (memchr(ptr, '"', sz) ? '\'' : '"');
         }

      command.reserve(command.size() + sz + 2);

      command.snprintf_add(" %c%.*s%c ", c, sz, ptr, c);
      }
}

bool UHTTP::processCGIOutput()
{
   U_TRACE(0, "UHTTP::processCGIOutput()")

   /*
   CGI Script Output

   The script sends its output to stdout. This output can either be a document generated by the script, or instructions to the server
   for retrieving the desired output.

   Script naming conventions

   Normally, scripts produce output which is interpreted and sent back to the client. An advantage of this is that the scripts do not
   need to send a full HTTP/1.0 header for every request.

   Some scripts may want to avoid the extra overhead of the server parsing their output, and talk directly to the client. In order to
   distinguish these scripts from the other scripts, CGI requires that the script name begins with nph- if a script does not want the
   server to parse its header. In this case, it is the script's responsibility to return a valid HTTP response to the client.

   Parsed headers

   The output of scripts begins with a small header. This header consists of text lines, in the same format as an HTTP header,
   terminated by a blank line (a line with only a linefeed or CR/LF).

   Any headers which are not server directives are sent directly back to the client. Currently, this specification defines three server
   directives:

   * Content-type: This is the MIME type of the document you are returning.

   * Location:     This is used to specify to the server that you are returning a reference to a document rather than an actual document.
                   If the argument to this is a URL, the server will issue a redirect to the client.

   * Status:       This is used to give the server an HTTP status line to send to the client. The format is nnn xxxxx, where nnn is
                   the 3-digit status code, and xxxxx is the reason string, such as "Forbidden".
   */

   uint32_t sz = UClientImage_Base::wbuffer->size();

   if (sz == 0)
      {
      U_SRV_LOG_MSG("UHTTP::processCGIOutput(): the write buffer (cgi response for client) is empty...");

      UHTTP::setHTTPInternalError(); // set internal error response...

      U_RETURN(false);
      }

   const char* ptr    = UClientImage_Base::wbuffer->data();
   uint32_t endHeader = u_findEndHeader(ptr, sz);

   // NB: endHeader comprende anche la blank line...

   U_INTERNAL_DUMP("endHeader = %u u_line_terminator_len = %d", endHeader, u_line_terminator_len);

   if (endHeader == U_NOT_FOUND) U_RETURN(false);

   UString set_cookie;
   const char* location;
   int nResponseCode = HTTP_OK;
   bool connection_close = false;

   U_INTERNAL_DUMP("ptr = %.20S", ptr)

   // check for script's responsibility to return a valid HTTP response to the client...

   if (scanfHTTPHeader(ptr))
      {
      U_INTERNAL_DUMP("wbuffer(%u) = %#.*S", UClientImage_Base::wbuffer->size(), U_STRING_TO_TRACE(*UClientImage_Base::wbuffer));

      U_INTERNAL_DUMP("http_info.is_connection_close = %d", http_info.is_connection_close);

      if (http_info.is_connection_close == U_MAYBE)
         {
         http_info.is_connection_close = (u_find(ptr + 15, endHeader, U_CONSTANT_TO_PARAM("Connection: close")) ? U_YES : U_NOT);

         U_INTERNAL_DUMP("http_info.is_connection_close = %d", http_info.is_connection_close);
         }

      U_RETURN(true);
      }

   // check if is used to specify to the server to close the connection...

   U_INTERNAL_DUMP("check 'Connection: close'", 0)

   if (U_STRNCMP(ptr, "Connection: close") == 0)
      {
      ptr += U_CONSTANT_SIZE("Connection: close") + u_line_terminator_len;

      connection_close = true;

      U_INTERNAL_DUMP("ptr = %.20S", ptr)
      }

   // check for the MIME type of the document you are returning...

   U_INTERNAL_DUMP("check 'Content-Type: '", 0)

   if (U_STRNCMP(ptr, "Content-Type: ") == 0) goto send_cgi_response;

   // ULIB facility: check for request TODO stateless session cookies...

   U_INTERNAL_DUMP("check 'Set-Cookie: TODO['", 0)

   if (U_STRNCMP(ptr, "Set-Cookie: TODO[") == 0)
      {
      ptr     += U_CONSTANT_SIZE("Set-Cookie: TODO[");
      location = ptr - 1;

      while (*ptr++ != ']') {}

      // REQ: Set-Cookie: TODO[ data expire path domain secure HttpOnly ]
      // ----------------------------------------------------------------------------------------------------------------------------
      // string -- Data to put into the cookie        -- must
      // int    -- Lifetime of the cookie in HOURS    -- must (0 -> valid until browser exit)
      // string -- Path where the cookie can be used  --  opt
      // string -- Domain which can read the cookie   --  opt
      // bool   -- Secure mode                        --  opt
      // bool   -- Only allow HTTP usage              --  opt
      // ----------------------------------------------------------------------------------------------------------------------------
      // RET: Set-Cookie: ulib_sid=data&expire&HMAC-MD5(data&expire); expires=expire(GMT); path=path; domain=domain; secure; HttpOnly

      uint32_t len = ptr - location;

      set_cookie = setHTTPCookie(UClientImage_Base::wbuffer->substr(location, len));

      U_INTERNAL_DUMP("set_cookie = %.*S", U_STRING_TO_TRACE(set_cookie))

      // for next parsing...

      while (*ptr++ != '\n') {}

      U_INTERNAL_DUMP("ptr = %.20S", ptr)

      // check for the MIME type of the document you are returning...

      if (U_STRNCASECMP(ptr, "Content-Type: ") == 0)
         {
         uint32_t pos  = U_CONSTANT_SIZE("Set-Cookie: "),
                   n1  = U_CONSTANT_SIZE("TODO") + len + 1, // '\n'...
                   n2  = set_cookie.size() - pos,
                  diff = n2 - n1;

         sz        += diff;
         endHeader += diff;

         U_INTERNAL_DUMP("diff = %u sz = %u endHeader = %u", diff, sz, endHeader)

         U_INTERNAL_ASSERT_MINOR(diff,512)

         (void) UClientImage_Base::wbuffer->replace(pos, n1, set_cookie, pos, n2);

         U_INTERNAL_DUMP("wbuffer(%u) = %#.*S", UClientImage_Base::wbuffer->size(), U_STRING_TO_TRACE(*UClientImage_Base::wbuffer));

         U_ASSERT_EQUALS(sz, UClientImage_Base::wbuffer->size())

         goto send_cgi_response;
         }
      }

   // check if is used to specify to the server that you are returning a reference to a document...

   U_INTERNAL_DUMP("check 'Location: '", 0)

   if (U_STRNCMP(ptr, "Location: ") == 0)
      {
      location = ptr + U_CONSTANT_SIZE("Location: ");

      ptr = strchr(location, (u_line_terminator_len == 1 ? '\n' : '\r'));

      U_INTERNAL_ASSERT_POINTER(ptr)

      *UClientImage_Base::wbuffer = getHTTPRedirectResponse(set_cookie, location, ptr - location);

      U_RETURN(true);
      }

   // check if is used to give the server an HTTP status line to send to the client...
   // ---------------------------------------------------------------------------------------------------------------------------------------------
   // Ex: "Status: 503 Service Unavailable\r\nX-Powered-By: PHP/5.2.6-pl7-gentoo\r\nContent-Type: text/html;charset=utf-8\r\n\r\n<!DOCTYPE html"...

   U_INTERNAL_DUMP("check 'Status: '", 0)

   if (U_STRNCMP(ptr, "Status: ") == 0)
      {
      location = ptr + U_CONSTANT_SIZE("Status: ");

      nResponseCode = strtol(location, 0, 0);

      U_INTERNAL_DUMP("nResponseCode = %d", nResponseCode)

      location = strchr(location, '\n');

      U_INTERNAL_ASSERT_POINTER(location)

      uint32_t diff = location - ptr + 1; // NB: we cut also \n...

      U_INTERNAL_ASSERT_MINOR(diff,endHeader)

      sz        -= diff;
      endHeader -= diff;

      U_INTERNAL_DUMP("diff = %u sz = %u endHeader = %u", diff, sz, endHeader)

      U_INTERNAL_ASSERT_MINOR(diff,512)

      UClientImage_Base::wbuffer->erase(0, location - ptr + 1); // NB: we cut also \n...

      goto send_cgi_response;
      }

   // check for the MIME type of the document you are returning... or other generic header...

   if (U_STRNCASECMP(ptr, "Content-Type: ")  == 0 ||
       U_STRNCASECMP(ptr, "Cache-Control: ") == 0 ||
       U_STRNCASECMP(ptr, "X-")              == 0 || // eXperimental HTTP header...
       U_STRING_FIND_EXT(*UClientImage_Base::wbuffer, 15, "Content-Type: ", endHeader) != U_NOT_FOUND)
      {
send_cgi_response:

      U_INTERNAL_ASSERT_MAJOR(endHeader, 0)

      uint32_t pos               = UClientImage_Base::wbuffer->find(*USocket::str_content_length, 0, endHeader),
               content_length    = sz - endHeader;
      bool header_content_length = (pos != U_NOT_FOUND);

      if (pos != U_NOT_FOUND)
         {
         pos += USocket::str_content_length->size() + 2;

               char* endptr;
         const char* nptr = UClientImage_Base::wbuffer->c_pointer(pos);
         uint32_t clength = (uint32_t) strtoul(nptr, &endptr, 0);

         // NB: with drupal happens...!!!

         if (clength != content_length)
            {
            char bp[12];

            U_INTERNAL_DUMP("clength = %u content_length = %u", clength, content_length);

            (void) UClientImage_Base::wbuffer->replace(pos, (endptr - nptr), bp, u_snprintf(bp, sizeof(bp), "%u", content_length));

            U_INTERNAL_DUMP("wbuffer(%u) = %#.*S", UClientImage_Base::wbuffer->size(), U_STRING_TO_TRACE(*UClientImage_Base::wbuffer));
            }
         }

      setHTTPCgiResponse(nResponseCode, content_length, header_content_length);

      if (connection_close) http_info.is_connection_close = U_YES;

      U_RETURN(true);
      }

   U_RETURN(false);
}

bool UHTTP::processCGIRequest(UCommand* pcmd, UString* penvironment)
{
   U_TRACE(0, "UHTTP::processCGIRequest(%p,%p)", pcmd, penvironment)

   U_ASSERT(isPHPRequest() || isCGIRequest())

   // process the CGI request

   U_INTERNAL_DUMP("method = %.*S method_type = %u uri = %.*S", U_HTTP_METHOD_TO_TRACE, http_info.method_type, U_HTTP_URI_TO_TRACE)

   UCommand cmd;
   bool sh_script;
   UString command(U_CAPACITY), environment(U_CAPACITY);

   if (pcmd)
      {
      command     = pcmd->getStringCommand();
      environment = pcmd->getStringEnvironment();
      sh_script   = pcmd->isShellScript();
      }
   else
      {
      pcmd = &cmd;

      (void) command.assign(file->getPath()); // NB: we can't use relativ path because after we call chdir()...

      if (penvironment) environment = *penvironment;

      sh_script = UStringExt::endsWith(command, U_CONSTANT_TO_PARAM(".sh"));

      if      (sh_script)                                                    (void) command.insert(0, U_PATH_SHELL);
      else if (UHTTP::isPHPRequest())                                        (void) command.insert(0, U_CONSTANT_TO_PARAM("php-cgi "));
   // else if (u_endsWith(U_HTTP_URI_TO_PARAM, U_CONSTANT_TO_PARAM(".pl")))  (void) command.insert(0, U_CONSTANT_TO_PARAM("perl "));
   // else if (u_endsWith(U_HTTP_URI_TO_PARAM, U_CONSTANT_TO_PARAM(".py")))  (void) command.insert(0, U_CONSTANT_TO_PARAM("python "));
   // else if (u_endsWith(U_HTTP_URI_TO_PARAM, U_CONSTANT_TO_PARAM(".rb")))  (void) command.insert(0, U_CONSTANT_TO_PARAM("ruby "));
      }

   if (sh_script == false &&
       (pcmd->setCommand(command), pcmd->checkForExecute()) == false)
      {
      // set forbidden error response...

      setHTTPForbidden();

      U_RETURN(false);
      }

#ifdef DEBUG
   static int fd_stderr = UFile::creat("/tmp/processCGIRequest.err", O_WRONLY | O_APPEND, PERM_FILE);
#else
   static int fd_stderr = UServices::getDevNull();
#endif

   // NB: if server no preforked (ex: nodog) process the HTTP CGI request with fork....

   if (UServer_Base::preforked_num_kids == 0)
      {
      U_INTERNAL_ASSERT_POINTER(UServer_Base::proc)

      if (UServer_Base::proc->fork() &&
          UServer_Base::proc->parent())
         {
         UClientImage_Base::write_off = true;

         (void) UProcess::waitpid(); // per evitare piu' di 1 zombie...

         U_RETURN(false);
         }

      if (UServer_Base::proc->child())
         {
         UServer_Base::flag_loop = false;

         if (UServer_Base::isLog())
            {
            u_unatexit(&ULog::close); // NB: needed because all instance try to close the log... (inherits from its parent)

            if (ULog::isMemoryMapped() &&
                ULog::isShared() == false)
               {
               // NB: we need locking to write log...

               UServer_Base::log = 0;
               }
            }
         }
      }

   // ULIB facility: check if present form data and convert it in parameters for shell script...

   if (sh_script) setCGIShellScript(command);

   setCGIEnvironment(environment, sh_script);

   pcmd->set(command, &environment);

   /* When a url ends by "/cgi-bin/" it is assumed to be a cgi script.
    * The server changes directory to the location of the script and
    * executes it after setting QUERY_STRING and other environment variables.
    */

   bool result = false;
   char cgi_dir[PATH_MAX];
   uint32_t sz = http_info.uri_len - 1;
   const char* ptr = U_HTTP_URI + sz;

   while (*ptr != '/')
      {
      --sz;
      --ptr;
      }

   while (*ptr == '/')
      {
      --sz;
      --ptr;
      }

   (void) U_SYSCALL(memcpy, "%p,%p,%u", cgi_dir, U_HTTP_URI + 1, sz);

   cgi_dir[sz] = '\0';

   U_INTERNAL_DUMP("cgi_dir = %S", cgi_dir)

   if (UFile::chdir(cgi_dir, true))
      {
      // execute CGI script...

      UString* pinput = (UClientImage_Base::body->empty() ? 0 : UClientImage_Base::body);

      result = pcmd->execute(pinput, UClientImage_Base::wbuffer, -1, fd_stderr);

      (void) UFile::chdir(0, true);
      }

   resetForm();

   // process the HTTP CGI output

   if (result == false) UServer_Base::logCommandMsgError(pcmd->getCommand());

   if (processCGIOutput() == false)
      {
      setHTTPInternalError(); // set internal error response...

      U_RETURN(false);
      }

   U_RETURN(true);
}

#define U_NO_ETAG // for me it's enough Last-Modified: ...

bool UHTTP::processHTTPGetRequest()
{
   U_TRACE(0, "UHTTP::processHTTPGetRequest()")

   U_ASSERT(UClientImage_Base::body->empty())

   UString ext(300U), etag;
   const char* content_type;
   int nResponseCode = HTTP_OK;
   off_t start = 0, size = 0, end = 0;
   bool bdir = false, range = false, result = true, isSSL = false;

   /*
   The If-Modified-Since: header is used with a GET request. If the requested resource has been modified since the given date,
   ignore the header and return the resource as you normally would. Otherwise, return a "304 Not Modified" response, including
   the Date: header and no message body, like

   HTTP/1.1 304 Not Modified
   Date: Fri, 31 Dec 1999 23:59:59 GMT
   [blank line here]
   */

   const char* ptr = getHTTPHeaderValuePtr(*USocket::str_if_modified_since);

   if (ptr)
      {
      time_t since = UDate::getSecondFromTime(ptr, true);

      U_INTERNAL_DUMP("since          = %u", since)
      U_INTERNAL_DUMP("file->st_mtime = %u", file->st_mtime)

      if (file->st_mtime <= since)
         {
         /* All 1xx (informational), 204 (no content), and 304 (not modified) responses must not include a body. All other responses must include an
          * entity body or a Content-Length header field defined with a value of zero (0)
          */

         *UClientImage_Base::wbuffer = getHTTPHeaderForResponse(HTTP_NOT_MODIFIED, 0, UString::getStringNull());

         goto send_header;
         }
      }
   /* I think it's not very much used...
   else
      {
      The If-Unmodified-Since: header is similar, but can be used with any method. If the requested resource has not been modified
      since the given date, ignore the header and return the resource as you normally would. Otherwise, return a "412 Precondition Failed"
      response, like

      HTTP/1.1 412 Precondition Failed
      [blank line here]

      ptr = getHTTPHeaderValuePtr(*USocket::str_if_unmodified_since);

      if (ptr)
         {
         time_t since = UDate::getSecondFromTime(ptr, true);

         U_INTERNAL_DUMP("since          = %u", since)
         U_INTERNAL_DUMP("file->st_mtime = %u", file->st_mtime)

         if (file->st_mtime > since)
            {
            http_info.is_connection_close = U_YES;

            *UClientImage_Base::wbuffer = getHTTPHeaderForResponse(HTTP_PRECON_FAILED, 0, *UClientImage_Base::body);

            result = false;

            goto send_header;
            }
         }
      }
   */

#ifndef U_NO_ETAG
   // If-None-Match: Later, if the browser has to validate a component, it uses the If-None-Match header to pass the ETag back to
   // the origin server. If the ETags match, a 304 status code is returned reducing the response...

   etag = file->etag();

   ptr = getHTTPHeaderValuePtr(*USocket::str_if_none_match);

   if (ptr)
      {
      U_INTERNAL_ASSERT_EQUALS(*ptr, '"') // entity-tag

      if (etag.equal(ptr, etag.size()))
         {
         /* All 1xx (informational), 204 (no content), and 304 (not modified) responses must not include a body. All other responses must include an
          * entity body or a Content-Length header field defined with a value of zero (0)
          */

         *UClientImage_Base::wbuffer = getHTTPHeaderForResponse(HTTP_NOT_MODIFIED, 0, UString::getStringNull());

         goto send_header;
         }
      }
#endif

   if (openFile() == false)
      {
      // set forbidden error response...

      setHTTPForbidden();

      result = false;

      goto send_header;
      }

   if (file->dir())
      {
      bdir = true;

      // check if it's OK to do directory listing via authentication (digest|basic)

      if (processHTTPAuthorization(UServer_Base::digest_authentication, UString(U_HTTP_URI_TO_PARAM)) == false)
         {
         setHTTPUnAuthorized(UServer_Base::digest_authentication);

         result = false;

         goto send_header;
         }

      *UClientImage_Base::body = getHTMLDirectoryList();
      size                     = UClientImage_Base::body->size();
      content_type             = U_CTYPE_HTML;
      }
   else
      {
      size         = file->getSize();
      end          = size - 1;
      content_type = "application/octet-stream";

      // The Range: header is used with a GET request.
      // For example assume that will return only a portion (let's say the first 32 bytes) of the requested resource...
      //
      // Range: bytes=0-31

      ptr = getHTTPHeaderValuePtr(*USocket::str_range);

      if (ptr &&
          U_STRNCMP(ptr, "bytes=") == 0)
         {
         ptr += U_CONSTANT_SIZE("bytes=");

         /* Only support %d- and %d-%d, not %d-%d,%d-%d or -%d
          *
          * NB: we have problem with use of sscanf()... (format -> "bytes=%ld-%ld")
          */

         start = (u_isdigit(*ptr) ? u_strtooff(ptr, &ptr, 0) : 0);

         U_INTERNAL_DUMP("ptr = %.*S", 10, ptr)

         end = (*ptr == '-' && u_isdigit(*++ptr) ? u_strtooff(ptr, &ptr, 0) : 0);

         U_INTERNAL_DUMP("ptr = %.*S", 10, ptr)

         range = (*ptr != ',' && start < end && start < size);

         U_INTERNAL_DUMP("start = %I end = %I range = %b", start, end, range)

         if (range == false)
            {
            http_info.is_connection_close = U_YES;

            *UClientImage_Base::wbuffer = getHTTPHeaderForResponse(HTTP_REQ_RANGE_NOT_OK, 0, *UClientImage_Base::body);

            result = false;

            goto send_header;
            }

         // The If-Range: header allows a client to "short-circuit" the request (conditional GET). Informally, its meaning is
         // `if the entity is unchanged, send me the part(s) that I am missing; otherwise, send me the entire new entity'.
         //
         // If-Range: ( entity-tag | HTTP-date )
         //
         // If the client has no entity tag for an entity, but does have a Last-Modified date, it MAY use that date in an If-Range header.
         // (The server can distinguish between a valid HTTP-date and any form of entity-tag by examining no more than two characters.)
         // The If-Range header SHOULD only be used together with a Range header, and MUST be ignored if the request does not include a
         // Range header, or if the server does not support the sub-range operation. 

         ptr = getHTTPHeaderValuePtr(*USocket::str_if_range);

         if (ptr)
            {
#        ifndef U_NO_ETAG
            if (*ptr == '"') range = (etag.equal(ptr, etag.size())); // entity-tag
            else // HTTP-date
#        endif
               {
               time_t since = UDate::getSecondFromTime(ptr, true);

               U_INTERNAL_DUMP("since          = %u", since)
               U_INTERNAL_DUMP("file->st_mtime = %u", file->st_mtime)

               range = (file->st_mtime <= since);
               }
            }
         }
      }

   U_INTERNAL_DUMP("range = %b", range)

   // build response...

#ifndef U_NO_ETAG
   ext.snprintf("Etag: %.*s\r\n", U_STRING_TO_TRACE(etag));
#endif

   ext.snprintf_add("Last-Modified: %#7D GMT\r\n", file->st_mtime);

   if (bdir)
      {
      if (isHTTPAcceptEncodingDeflate(size))
         {
         *UClientImage_Base::body = UStringExt::deflate(*UClientImage_Base::body);
         size                     = UClientImage_Base::body->size();

         (void) ext.append(U_CONSTANT_TO_PARAM("Content-Encoding: deflate\r\n"));
         }

      goto next;
      }

   if (range)
      {
      if (end >= size) end = size - 1;

      U_INTERNAL_ASSERT_RANGE(0,start,end-1)
      U_INTERNAL_ASSERT_RANGE(start+1,end,size-1)

      ext.snprintf_add("Content-Range: bytes %I-%I/%I\r\n", start, end, size);

      size          = end - start + 1;
      nResponseCode = HTTP_PARTIAL;
      }

   if (size                                                == 0 ||
       file->memmap(PROT_READ, UClientImage_Base::wbuffer) == false)
      {
      goto next;
      }

   if (u_endsWith(U_FILE_TO_PARAM(*file), U_CONSTANT_TO_PARAM(".css")))
      {
      content_type = "text/css";

      (void) ext.append("Content-Style-Type: text/css\r\n");

      if (u_endsWith(U_FILE_TO_PARAM(*file), U_CONSTANT_TO_PARAM(".compressed.css")))
         {
         (void) ext.append(U_CONSTANT_TO_PARAM("Content-Encoding: gzip\r\n"));
         }
      }
   else if (u_endsWith(U_FILE_TO_PARAM(*file), U_CONSTANT_TO_PARAM(".js")))
      {
      content_type = "text/javascript";

      (void) ext.append("Content-Script-Type: text/javascript\r\n");

      if (u_endsWith(U_FILE_TO_PARAM(*file), U_CONSTANT_TO_PARAM(".compressed.js")))
         {
         (void) ext.append(U_CONSTANT_TO_PARAM("Content-Encoding: gzip\r\n"));
         }
      }
   else
      {
      content_type = file->getMimeType();
      }

next:

   ext.snprintf_add("Content-Type: %s\r\n"
                    "Content-Length: %u\r\n"
                    "\r\n",
                    content_type, size);

#ifdef HAVE_SSL
   if (UClientImage_Base::socket->isSSL())
      {
      isSSL = true;

      if (bdir == false) *UClientImage_Base::body = *UClientImage_Base::wbuffer;
      }
#endif

   *UClientImage_Base::wbuffer = getHTTPHeaderForResponse(nResponseCode, ext); // NB: this assignment make also the unmmap() of file...

   /* On Linux, sendfile() depends on the TCP_CORK socket option to avoid undesirable packet boundaries
    * ------------------------------------------------------------------------------------------------------
    * if we set the TCP_CORK option on the socket, our header packet will be padded with the bulk data
    * and all the data will be transferred automatically in the packets according to size. When finished
    * with the bulk data transfer, it is advisable to uncork the connection by unsetting the TCP_CORK option
    * so that any partial frames that are left can go out. This is equally important to corking. To sum it up,
    * we recommend setting the TCP_CORK option when you're sure that you will be sending multiple data sets
    * together (such as header and a body of HTTP response), with no delays between them. This can greatly
    * benefit the performance of WWW, FTP, and file servers, as well as simplifying your life
    * ------------------------------------------------------------------------------------------------------
    */

   if (UServer_Base::useTcpOptimization()) UClientImage_Base::socket->setTcpCork(1U);

send_header:

   if (UClientImage_Base::pClientImage->handlerWrite() == U_NOTIFIER_OK &&
       result                                                           &&
       size                                                             &&
       isHttpHEAD() == false)
      {
      // send body...

      if (isSSL || bdir)
         {
         if (USocketExt::write(UClientImage_Base::socket, UClientImage_Base::body->c_pointer(start), size) == false) result = false;

         UClientImage_Base::body->clear(); // NB: this make also the unmmap() of file...
         }
      else
         {
         if (file->sendfile(UClientImage_Base::socket->getFd(), &start, size) == false) result = false;
         }

      if (UServer_Base::useTcpOptimization()) UClientImage_Base::socket->setTcpCork(0U);
      }

   if (file->isOpen()) file->close();

   UClientImage_Base::write_off = true;

   U_RETURN(result);
}
