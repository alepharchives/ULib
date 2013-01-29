// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    http.cpp - client HTTP
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/file.h>
#include <ulib/utility/base64.h>
#include <ulib/net/client/http.h>
#include <ulib/utility/services.h>

#ifdef USE_LIBMAGIC
#  include <ulib/magic/magic.h>
#endif

/*
An Example with HTTP/1.0 
==========================================================================================
server{meng}% telnet www.cs.panam.edu 80
Trying 129.113.132.240...
Connected to server.cs.panam.edu.
Escape character is '^]'.
GET /index.html HTTP/1.0
From: meng@panam.edu
User-Agent: Test/1.1

HTTP/1.1 200 OK
Date: Tue, 13 Apr 1999 19:30:29 GMT
Server: Apache/1.3.2 (Unix)
Last-Modified: Wed, 03 Feb 1999 22:06:29 GMT
ETag: "5beed-b41-36b8c865"
Accept-Ranges: bytes
Content-Length: 2881
Connection: close
Content-Type: text/html

<!DOCTYPE HTML SYSTEM "html.dtd">
<HTML>
<HEAD>
<TITLE>Department of Computer Science UT-Pan American</TITLE>
</HEAD>
<BODY>
<IMG SRC="/LocalIcons/utpa_g_t.gif"  HSPACE=15 VSPACE=0 BORDER=0 ALIGN=left >
<!--width=150 height=120--!>
<H2>
<BR>
Department Of Computer Science
</H2>
<BR  CLEAR=LEFT>
<HR>

<H2>
<center>
Welcome</center>
</H2>
<h3>
<hr>
...

</BODY>
</HTML>
Connection closed by foreign host.
==========================================================================================
HTTP 1.1 is superset of HTTP 1.0. HTTP 1.1 added a few more requirements on both the server
side and the client side. On the client side:
---------------------------------------------------
1) include the "Host: ..." header with each request
---------------------------------------------------
Starting with HTTP 1.1, one server at one IP address can be multi-homed, i.e. the home of
several Web domains. For example, "www.host1.com" and "www.host2.com" can live on the same
server. That's why the Host field is required. Example:
GET /path/file.html HTTP/1.1
Host: www.host1.com:80
[blank line here]
-------------------------------------
2) accept responses with chunked data
-------------------------------------
If a server wants to start sending a response before knowing its total length (like with long
script output), it might use the simple chunked transfer-encoding, which breaks the complete
response into smaller chunks and sends them in series. You can identify such a response because
it contains the "Transfer-Encoding: chunked" header. All HTTP 1.1 clients must be able to receive
chunked messages. A chunked message body contains a series of chunks, followed by a line with a
single "0" (zero), followed by optional footers (just like headers), and a blank line. Each chunk
consists of two parts: a line with the size of the chunk data, in hex, possibly followed by a
semicolon and extra parameters you can ignore (none are currently standard), and ending with CRLF.
the data itself, followed by CRLF. An example:
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
or digital signatures.
-----------------------------------------------------------------------------------------------------
3) either support persistent connections, or include the "Connection: close" header with each request
-----------------------------------------------------------------------------------------------------
In HTTP 1.0 and before, TCP connections are closed after each request and response, so each resource to
be retrieved requires its own connection. Persistent connections are the default in HTTP 1.1, so nothing
special is required to use them. Just open a connection and send several requests in series (called pipelining),
and read the responses in the same order as the requests were sent. If you do this, be very careful to read
the correct length of each response, to separate them correctly. If a client includes the "Connection: close"
header in the request, then the connection will be closed after the corresponding response. Use this if you
don't support persistent connections, or if you know a request will be the last on its connection. Similarly,
if a response contains this header, then the server will close the connection following that response, and
the client shouldn't send any more requests through that connection. A server might close the connection
before all responses are sent, so a client must keep track of requests and resend them as needed. When
resending, don't pipeline the requests until you know the connection is persistent. Don't pipeline at all
if you know the server won't support persistent connections (like if it uses HTTP 1.0, based on a previous response).
-----------------------------------
4) handle the 100 Continue response
-----------------------------------
During the course of an HTTP 1.1 client sending a request to a server, the server might respond with an interim
"100 Continue" response. This means the server has received the first part of the request, and can be used to
aid communication over slow links. In any case, all HTT 1.1 clients must handle the 100 response correctly
(perhaps by just ignoring it). The "100 Continue" response is structured like any HTTP response, i.e. consists
of a status line, optional headers, and a blank line. Unlike other responses, it is always followed by another
complete, final response. Example:
HTTP/1.0 100 Continue
[blank line here]
HTTP/1.0 200 OK
Date: Fri, 31 Dec 1999 23:59:59 GMT
Content-Type: text/plain
Content-Length: 42
some-footer: some-value
another-footer: another-value

abcdefghijklmnoprstuvwxyz1234567890abcdef
To handle this, a simple HTTP 1.1 client might read one response from the socket; if the status code is 100,
discard the first response and read the next one instead.
-----------------------------------
To comply with HTTP 1.1, servers must:
--------------------------------------
1) require the Host: header from HTTP 1.1 clients
2) accept absolute URL's in a request
3) accept requests with chunked data
4) either support persistent connections, or include the "Connection: close" header with each response
5) use the "100 Continue" response appropriately
6) include the Date: header in each response
7) handle requests with If-Modified-Since: or If-Unmodified-Since: headers
8) support at least the GET and HEAD methods
9) support HTTP 1.0 requests
--------------------------------------
An example of transaction under HTTP 1.1:

server{meng}% telnet www.cs.panam.edu 80
Trying 129.113.132.240...
Connected to server.cs.panam.edu.
Escape character is '^]'.
GET /index.html HTTP/1.1
Host: www.cs.panam.edu:80
From: meng@panam.edu
User-Agent: test/1.1
Connection: close

HTTP/1.1 200 OK
Date: Tue, 13 Apr 1999 20:57:45 GMT
Server: Apache/1.3.2 (Unix)
Last-Modified: Wed, 03 Feb 1999 22:06:29 GMT
ETag: "5beed-b41-36b8c865"
Accept-Ranges: bytes
Content-Length: 2881
Connection: close
Content-Type: text/html

<! -------------------- Welcome.html ----------------------------
 
<!                 Depertment of Computer Science Home Page

<!DOCTYPE HTML SYSTEM "html.dtd">
<HTML>
<HEAD>
<TITLE>Department of Computer Science UT-Pan American</TITLE>
</HEAD>
<BODY>
<IMG SRC="/LocalIcons/utpa_g_t.gif"  HSPACE=15 VSPACE=0 BORDER=0 ALIGN=left >
<!--width=150 height=120--!>
<H2>
<BR>
Department Of Computer Science
</H2>
...
</BODY>
</HTML>
Connection closed by foreign host.
*/

#define U_MAX_REDIRECTS 10 // HTTP 1.0 used to suggest 5

const UString* UHttpClient_Base::str_www_authenticate;
const UString* UHttpClient_Base::str_proxy_authenticate;
const UString* UHttpClient_Base::str_proxy_authorization;

void UHttpClient_Base::str_allocate()
{
   U_TRACE(0, "UHttpClient_Base::str_allocate()")

   U_INTERNAL_ASSERT_EQUALS(str_www_authenticate, 0)
   U_INTERNAL_ASSERT_EQUALS(str_proxy_authenticate, 0)
   U_INTERNAL_ASSERT_EQUALS(str_proxy_authorization, 0)

   static ustringrep stringrep_storage[] = {
      { U_STRINGREP_FROM_CONSTANT("WWW-Authenticate") },
      { U_STRINGREP_FROM_CONSTANT("Proxy-Authenticate") },
      { U_STRINGREP_FROM_CONSTANT("Proxy-Authorization") }
   };

   U_NEW_ULIB_OBJECT(str_www_authenticate,    U_STRING_FROM_STRINGREP_STORAGE(0));
   U_NEW_ULIB_OBJECT(str_proxy_authenticate,  U_STRING_FROM_STRINGREP_STORAGE(1));
   U_NEW_ULIB_OBJECT(str_proxy_authorization, U_STRING_FROM_STRINGREP_STORAGE(2));

   if (UIPAddress::str_localhost == 0) UIPAddress::str_allocate();
}

UHttpClient_Base::UHttpClient_Base(UFileConfig* cfg) : UClient_Base(cfg)
{
   U_TRACE_REGISTER_OBJECT(0, UHttpClient_Base, "%p", cfg)

   requestHeader    = U_NEW(UMimeHeader);
   responseHeader   = U_NEW(UMimeHeader);
   nResponseCode    = 0;
   bFollowRedirects = true;

   if (str_www_authenticate == 0) str_allocate();
}

UHttpClient_Base::~UHttpClient_Base()
{
   U_TRACE_UNREGISTER_OBJECT(0, UHttpClient_Base)

   delete requestHeader;
   delete responseHeader;
}

void UHttpClient_Base::reset()
{
   U_TRACE(0, "UHttpClient_Base::reset()")

     body.clear();
   method.clear();

    requestHeader->clear();
   responseHeader->clear();

   UClient_Base::reset();
   UClient_Base::server.clear();
}

//=======================================================================================
// In response to a HTTP_UNAUTHORISED response from the HTTP server,
// this function will attempt to generate an Authentication header to satisfy the server.
//=======================================================================================

bool UHttpClient_Base::createAuthorizationHeader()
{
   U_TRACE(0, "UHttpClient_Base::createAuthorizationHeader()")

   bool bProxy = (u_http_info.nResponseCode == HTTP_PROXY_AUTH);

   UString sHeader = *(bProxy ? str_proxy_authenticate
                              : str_www_authenticate);

   UString authResponse = responseHeader->getHeader(sHeader);

   if (authResponse.empty())
      {
      U_DUMP("%.*S header missing from HTTP response: %d", U_STRING_TO_TRACE(sHeader), u_http_info.nResponseCode)

      U_RETURN(false);
      }

   if (    user.empty() ||
       password.empty())
      {
      // If the registered Authenticator cannot supply a user/password then we cannot continue.
      // This is signalled by returning false to the sendRequest() function.

      U_RETURN(false);
      }

   // The authentication header is constructed like a tagged attribute list (Ex.)
   // -------------------------------------------------------------------------------
   // WWW-Authenticate: Basic  realm="SokEvo"
   // WWW-Authenticate: Digest realm="Autenticazione su LDAP/SSL", nonce="GkPcSTxaBAA=666065cb86c557d75991c7b3fa362e7f881abb93", algorithm=MD5, qop="auth"

   UVector<UString> name_value;
   uint32_t n = name_value.split(authResponse, ",= ");

   if (n < 3)
      {
      U_WARNING("%S header value: %S is invalid", sHeader.data(), authResponse.data());

      U_RETURN(false);
      }

   UString scheme = name_value[0], headerValue(300U);

   // According to RFC 2617 HTTP Authentication: Basic and Digest Access Authentication
   // ---------------------------------------------------------------------------------------------------------------------------
   // For "Basic" authentication, the user and password are concatentated with a colon separator before being encoded in base64
   // According to RFC 2068 (HTTP/1.1) the Username and Password are defined as TEXT productions and are therefore supposed to be
   // encoded in ISO-8859-1 before being Base64-encoded

   if (scheme.equal(U_CONSTANT_TO_PARAM("Basic")))
      {
      UString tmp(100U), data(100U);

      tmp.snprintf("%.*s:%.*s", U_STRING_TO_TRACE(user), U_STRING_TO_TRACE(password));

      UBase64::encode(tmp, data);

      // Authorization: Basic cy5jYXNhenphOnN0ZWZhbm8x

      headerValue.snprintf("Basic %.*s", U_STRING_TO_TRACE(data));
      }
   else
      {
      // WWW-Authenticate: Digest realm="Autenticazione su LDAP/SSL", nonce="86c557d75991c7b3fa362e7f881abb93", algorithm=MD5, qop="auth"

      U_ASSERT(scheme.equal(U_CONSTANT_TO_PARAM("Digest")))

      uint32_t i = 1;
      UString name, value, realm, nonce, algorithm, qop; // "Quality of Protection" (qop)

      while (i < n)
         {
         name  = name_value[i++];
         value = name_value[i++];

         U_INTERNAL_DUMP("name = %.*S value = %.*S", U_STRING_TO_TRACE(name), U_STRING_TO_TRACE(value))

         switch (name.c_char(0))
            {
            case 'q':
               {
               if (name.equal(U_CONSTANT_TO_PARAM("qop")))
                  {
                  U_INTERNAL_ASSERT_EQUALS((bool)qop, false)

                  qop = value;
                  }
               }
            break;

            case 'r':
               {
               if (name.equal(U_CONSTANT_TO_PARAM("realm")))
                  {
                  U_INTERNAL_ASSERT_EQUALS((bool)realm, false)

                  realm = value;
                  }
               }
            break;

            case 'n':
               {
               if (name.equal(U_CONSTANT_TO_PARAM("nonce")))
                  {
                  U_INTERNAL_ASSERT_EQUALS((bool)nonce, false)

                  nonce = value;
                  }
               }
            break;

            case 'a':
               {
               if (name.equal(U_CONSTANT_TO_PARAM("algorithm")))
                  {
                  algorithm = value;

                  if (algorithm.equal("MD5") == false) U_RETURN(false);
                  }
               }
            break;
            }
         }

      if (  qop.empty() ||
          realm.empty() ||
          nonce.empty())
         {
         // We cannot continue. This is signalled by returning false to the sendRequest() function.

         U_RETURN(false);
         }

      static uint32_t nc; //  nonce: counter incremented by client
                          // cnonce: client generated random nonce (u_now)

      UString a1(100U), ha1(33U),                     // MD5(user : realm : password)
              a2(4 + 1 + UClient_Base::uri.size()),   //     method : uri
              ha2(33U),                               // MD5(method : uri)
              a3(200U), _response(33U);               // MD5(HA1 : nonce : nc : cnonce : qop : HA2)

      // MD5(user : realm : password)

      a1.snprintf("%.*s:%.*s:%.*s", U_STRING_TO_TRACE(user), U_STRING_TO_TRACE(realm), U_STRING_TO_TRACE(password));

      UServices::generateDigest(U_HASH_MD5, 0, a1, ha1, false);

      // MD5(method : uri)

      a2.snprintf("%.*s:%.*s", U_STRING_TO_TRACE(method), U_STRING_TO_TRACE(UClient_Base::uri));

      UServices::generateDigest(U_HASH_MD5, 0, a2, ha2, false);

      // MD5(HA1 : nonce : nc : cnonce : qop : HA2)

      a3.snprintf("%.*s:%.*s:%08u:%ld:%.*s:%.*s", U_STRING_TO_TRACE(ha1), U_STRING_TO_TRACE(nonce),
                                                   ++nc,                   u_now->tv_sec,
                                                   U_STRING_TO_TRACE(qop), U_STRING_TO_TRACE(ha2));

      UServices::generateDigest(U_HASH_MD5, 0, a3, _response, false);

      // Authorization: Digest username="s.casazza", realm="Protected Area", nonce="1222108408", uri="/ok", cnonce="dad0f85801e27b987d6dc59338c7bf99",
      //                       nc=00000001, response="240312fba053f6d687d10c90928f4af2", qop="auth", algorithm="MD5"

      headerValue.snprintf("Digest username=\"%.*s\", realm=%.*s, nonce=%.*s, uri=\"%.*s\", cnonce=\"%ld\", nc=%08u, response=\"%.*s\", qop=%.*s",
                           U_STRING_TO_TRACE(user), U_STRING_TO_TRACE(realm), U_STRING_TO_TRACE(nonce),
                           U_STRING_TO_TRACE(UClient_Base::uri), u_now->tv_sec, nc, U_STRING_TO_TRACE(_response), U_STRING_TO_TRACE(qop));

      if (algorithm.empty() == false) (void) headerValue.append(U_CONSTANT_TO_PARAM(", algorithm=\"MD5\""));
      }

   // Only basic and digest authentication is supported at present. By failing to create an authentication header,
   // and returning false, we signal to the caller that the authenticate response should be treated as an error.

   if (headerValue.empty()) U_RETURN(false);

   requestHeader->setHeader(*(bProxy ? str_proxy_authorization
                                     : USocket::str_authorization), headerValue);

   U_RETURN(true);
}

// category of response:
// ------------------------------------------------------------
// -1 indicates errors of some kind
//  0 redirects the client to another URL with another socket
//  1 redirects the client to another URL using the same socket
//  2 indicates an success, no redirects, ok to read body
// ------------------------------------------------------------

int UHttpClient_Base::checkResponse(int& redirectCount)
{
   U_TRACE(0, "UHttpClient_Base::checkResponse(%d)", redirectCount)

   U_DUMP("UHTTP::getStatus() = %S", UHTTP::getStatus())

   // check if you can use the same socket connection

   if (responseHeader->isClose()) U_http_is_connection_close = U_YES;

   // General category of response:
   //
   // 1xx indicates an informational message only
   // 2xx indicates success of some kind
   // 3xx redirects the client to another URL
   // 4xx indicates an error on the client's part
   // 5xx indicates an error on the server's part

   if (u_http_info.nResponseCode == HTTP_UNAUTHORIZED || // 401
       u_http_info.nResponseCode == HTTP_PROXY_AUTH)     // 407
      {
      // If we haven't already done so, attempt to create an Authentication header. If this fails
      // (due to application not passing the credentials), then we treat it as an error.
      // If we already have one then the server is rejecting it so we have an error anyway.

      if ((u_http_info.nResponseCode == HTTP_UNAUTHORIZED && requestHeader->containsHeader(*USocket::str_authorization)) ||
          (u_http_info.nResponseCode == HTTP_PROXY_AUTH   && requestHeader->containsHeader(*str_proxy_authorization))    ||
          createAuthorizationHeader() == false)
         {
         U_RETURN(-2);
         }

      // check if you can use the same socket connection

      U_RETURN(U_http_is_connection_close == U_YES ? 0 : 1);
      }

   if (bFollowRedirects)
      {
      UString refresh = responseHeader->getRefresh();

      if (refresh.empty()           == false           ||
          u_http_info.nResponseCode == HTTP_MOVED_PERM || // 301
          u_http_info.nResponseCode == HTTP_MOVED_TEMP)   // 302
         {
         // 3xx redirects the client to another URL

         if (++redirectCount > U_MAX_REDIRECTS)
            {
            U_INTERNAL_DUMP("REDIRECTION LIMIT REACHED...")

            U_RETURN(-1);
            }

         UString newLocation = responseHeader->getLocation();

         if (newLocation.empty() &&
                 refresh.empty() == false)
            {
            uint32_t pos = U_STRING_FIND(refresh, 0, "url=");

            if (pos != U_NOT_FOUND)
               {
               newLocation = refresh.substr(pos + U_CONSTANT_SIZE("url="));

               if (newLocation.isQuoted()) newLocation.unQuote();
               }
            }

         if (newLocation.empty())
            {
            U_INTERNAL_DUMP("LOCATION HEADER MISSING FROM HTTP REDIRECT RESPONSE")

            U_RETURN(-1);
            }

         if (newLocation.find(*UIPAddress::str_localhost) != U_NOT_FOUND)
            {
            U_INTERNAL_DUMP("LOCATION HEADER POINT TO LOCALHOST CAUSING DEADLOCK")

            U_RETURN(-1);
            }

         // New locations will possibly need different authentication, so we should reset
         // our origin authentication header (if any)

         requestHeader->removeHeader(*USocket::str_authorization);

         // Store cookie if present "Set-Cookie:"

         setcookie = responseHeader->getHeader(*USocket::str_setcookie);

         U_INTERNAL_DUMP("Set-Cookie: %.*S", U_STRING_TO_TRACE(setcookie))

         if (setcookie.empty() == false && bproxy) 
            {
            U_INTERNAL_DUMP("SET-COOKIE HEADER PRESENT FROM HTTP REDIRECT RESPONSE")

            U_RETURN(2); // no redirection, read body
            }

#     ifdef DEBUG
         uri.clear();             // NB: to avoid DEAD OF SOURCE STRING WITH CHILD ALIVE... (uri can be a substr of url)
         newLocation.duplicate(); // NB: to avoid DEAD OF SOURCE STRING WITH CHILD ALIVE...
#     endif

         // Combine the new location with our URL

         if (UClient_Base::setUrl(newLocation) == false &&
             U_http_is_connection_close        != U_YES)
            {
            U_RETURN(1); // you can use the same socket connection for the redirect
            }

         U_RETURN(0); // redirects the client to another URL with another socket
         }
      }

   U_RETURN(2); // indicates success, no redirects, ok to read body
}

bool UHttpClient_Base::connectServer(const UString& _url)
{
   U_TRACE(0, "UHttpClient_Base::connectServer(%.*S)", U_STRING_TO_TRACE(_url))

   UClient_Base::uri.clear();

   if (UClient_Base::setUrl(_url))
      {
      if (UClient_Base::isConnected()) UClient_Base::socket->close(); // NB: is changed server and/or port to connect...

      if (UClient_Base::connect() == false) U_RETURN(false);
      }

   bool result = UClient_Base::isConnected();

   U_RETURN(result);
}

//=============================================================================
// Send the http request to the remote host.
//
// The request is made up of a request line followed by request header fields. Ex:
//
// GET filename HTTP/1.1
// Host: hostname[:port]
// Connection: close
//
// The response from the server will contain a number of header
// fields followed by the requested data.
//
// Note: HTTP Redirection
// ----------------------
// By default we will follow HTTP redirects. These are communicated
// to us by a 3xx HTTP response code and the presence of a "Location" header
// field. A 3xx response code without a Location header is an error.
// Redirection may be an iterative process, so it continues until
// we receive a 200 OK response or the maximum number of redirects is exceeded.
//
// We do not process Location headers when accompanying a 200 OK response.
//=============================================================================

bool UHttpClient_Base::sendRequest(const UString& req)
{
   U_TRACE(0, "UHttpClient_Base::sendRequest(%.*S)", U_STRING_TO_TRACE(req))

   U_INTERNAL_ASSERT(req)

   uint32_t startHeader = req.find(U_CRLF, 0, 2) + 2;

   U_INTERNAL_ASSERT_RANGE(11, startHeader, 8192)

   const char* ptr = req.data();

   struct iovec iov[3] = { { (caddr_t)ptr,                        startHeader },
                           { 0, 0 },
                           { (caddr_t)ptr+startHeader, req.size()-startHeader } };

   bool result = sendRequest(iov, 3);

   U_RETURN(result);
}

bool UHttpClient_Base::sendRequest(struct iovec* iov, int iovcnt)
{
   U_TRACE(0, "UHttpClient_Base::sendRequest(%p,%d)", iov, iovcnt)

   const char* ptr;
   uint32_t startHeader;
   UString req, headers;
   struct uhttpinfo u_http_info_save;
   int result = -1, redirectCount = 0, sendCount = 0;
   bool esito = true, bSaveHttpInfo = UHTTP::isValidRequest();

   if (bSaveHttpInfo) u_http_info_save = u_http_info;

   if (iov)
      {
      // NB: we check if there is already a parsing of the HTTP request made by UClient_Base::setUrl()...

      U_INTERNAL_DUMP("UClient_Base::uri = %.*S", U_STRING_TO_TRACE(UClient_Base::uri))

      if (UClient_Base::uri.empty())
         {
         if (UHTTP::scanfHeader((const char*)iov[0].iov_base, iov[0].iov_len) == false) U_RETURN(false);

         (void)            method.assign(U_HTTP_METHOD_TO_PARAM);
         (void) UClient_Base::uri.assign(U_HTTP_URI_QUERY_TO_PARAM);
         }
      }
   else
      {
      // we need to compose the request to the HTTP server...

      static struct iovec _iov[3];

      method = U_STRING_FROM_CONSTANT("GET");
compose:
      req = UClient_Base::wrapRequestWithHTTP(0, U_STRING_TO_PARAM(method), U_STRING_TO_PARAM(UClient_Base::uri), "\r\n");

      startHeader = req.find(U_CRLF, 0, 2) + 2;

      U_INTERNAL_ASSERT_RANGE(11, startHeader, 8192)

      iov    = _iov;
      iovcnt = 3;

      ptr = req.data();

      iov[0].iov_base = (caddr_t)ptr;
      iov[0].iov_len  = startHeader;
      iov[1].iov_base = 0;
      iov[1].iov_len  = 0;
      iov[2].iov_base = (caddr_t)ptr + startHeader;
      iov[2].iov_len  = req.size()   - startHeader;
      }

loop:
   if (result == 0 || // redirects the client to another URL with another socket
       result == 1)   // you can use the same socket connection for the redirect
      {
      result = -1;

      goto compose;
      }

   // check if there are some headers (Ex. Authentication) to insert in the request...

   if (requestHeader->empty() == false)
      {
      headers = requestHeader->getHeaders();

      iov[1].iov_base = (caddr_t)headers.data();
      iov[1].iov_len  =          headers.size();
      }

              body.clear();
   responseHeader->clear();

   UClient_Base::response.setEmpty();

   // write the request to the HTTP server and
   // read the response header of the HTTP server

   result = (UClient_Base::sendRequest(iov, iovcnt, true) &&
             responseHeader->readHeader(socket, UClient_Base::response)
                  ? checkResponse(redirectCount)
                  : -1);

   if (result ==  1) goto loop; // redirection, use the same socket connection...
   if (result == -2) goto end;  // pass HTTP_UNAUTHORISED response to the HTTP client...
   if (result ==  2)            // no redirection, read body...
      {
      U_DUMP("SERVER RETURNED HTTP RESPONSE: %d", u_http_info.nResponseCode)

      u_http_info.clength = responseHeader->getHeader(*USocket::str_content_length).strtol();

      if ((u_http_info.clength == 0                                 &&
           (U_http_chunked = responseHeader->isChunked()) == false) ||
           UHTTP::readBody(socket, &response, body))
         {
         goto end;
         }

      if (u_http_info.nResponseCode == HTTP_CLIENT_TIMEOUT ||
          u_http_info.nResponseCode == HTTP_ENTITY_TOO_LARGE)
         {
         esito = false;

         goto end;
         }
      }

   if (result < 0       &&
       (++sendCount > 5 ||
        UClient_Base::socket->isConnected() == false))
      {
      // NB: same error or may be we are in a loop...

      esito = false;

      goto end;
      }

   UClient_Base::socket->close();

   goto loop;

end:
   nResponseCode = u_http_info.nResponseCode;

   if (bSaveHttpInfo) u_http_info = u_http_info_save;
   else               U_HTTP_INFO_INIT(0);

   U_RETURN(esito);
}

bool UHttpClient_Base::sendPost(const UString& _url, const UString& _body, const char* content_type)
{
   U_TRACE(0, "UHttpClient_Base::sendPost(%.*S,%.*S,%S)", U_STRING_TO_TRACE(_url), U_STRING_TO_TRACE(_body), content_type)

   if (connectServer(_url) == false)
      {
      body = UClient_Base::response;

      U_RETURN(false);
      }

   U_INTERNAL_ASSERT(UClient_Base::uri)

   uint32_t sz = _body.size();
   UString post(UClient_Base::uri.size() + UClient_Base::server.size() + 300U);

   post.snprintf("POST %.*s HTTP/1.1\r\n"
                 "Host: %.*s:%d\r\n"
                 "User-Agent: ULib/1.0\r\n"
                 "Content-Length: %d\r\n"
                 "Content-Type: %s\r\n"
                 "\r\n",
                 U_STRING_TO_TRACE(UClient_Base::uri),
                 U_STRING_TO_TRACE(UClient_Base::server), UClient_Base::port,
                 sz,
                 content_type);

   // send request to server and get response

   uint32_t startHeader = post.find(U_CRLF, 0, 2) + 2;

   U_INTERNAL_ASSERT_RANGE(11, startHeader, 8192)

   const char* ptr = post.data();

   struct iovec iov[4] = { { (caddr_t)ptr,                         startHeader },
                           { 0, 0 },
                           { (caddr_t)ptr+startHeader, post.size()-startHeader },
                           { (caddr_t)_body.data(), sz } };

   method = U_STRING_FROM_CONSTANT("POST");

   if (sendRequest(iov, 4)) U_RETURN(true);

   U_RETURN(false);
}

bool UHttpClient_Base::upload(const UString& _url, UFile& file)
{
   U_TRACE(0, "UHttpClient_Base::upload(%.*S,%.*S)", U_STRING_TO_TRACE(_url), U_FILE_TO_TRACE(file))

   if (connectServer(_url) == false)
      {
      body = UClient_Base::response;

      U_RETURN(false);
      }

   UString content = file.getContent();

   if (content.empty()) U_RETURN(false);

   U_INTERNAL_ASSERT(UClient_Base::uri)

#ifdef USE_LIBMAGIC
   if (UMagic::magic == 0) (void) UMagic::init();
#endif

   uint32_t sz = content.size();
   UString _body(file.getPathRelativLen() + 300U),
           post(UClient_Base::uri.size() + UClient_Base::server.size() + 300U);

   _body.snprintf("------------------------------b34551106891\r\n"
                  "Content-Disposition: form-data; name=\"file\"; filename=\"%.*s\"\r\n"
                  "Content-Type: %s\r\n"
                  "\r\n",
                  U_FILE_TO_TRACE(file),
                  file.getMimeType(false));

   struct iovec iov[6] = { { 0, 0 },
                           { 0, 0 },
                           { 0, 0 },
                           { (caddr_t)_body.data(), _body.size() },
                           { (caddr_t)content.data(), sz },
                           { (caddr_t) U_CONSTANT_TO_PARAM("\r\n"
                                                           "------------------------------b34551106891--\r\n") } };

   post.snprintf("POST %.*s HTTP/1.1\r\n"
                 "Host: %.*s:%d\r\n"
                 "User-Agent: ULib/1.0\r\n"
                 "Content-Length: %d\r\n"
                 "Content-Type: multipart/form-data; boundary=----------------------------b34551106891\r\n"
                 "\r\n",
                 U_STRING_TO_TRACE(UClient_Base::uri),
                 U_STRING_TO_TRACE(UClient_Base::server), UClient_Base::port,
                 _body.size() + sz + iov[5].iov_len);

   // send upload request to server and get response

   uint32_t startHeader = post.find(U_CRLF, 0, 2) + 2;

   U_INTERNAL_ASSERT_RANGE(11, startHeader, 8192)

   const char* ptr = post.data();

   iov[0].iov_base = (caddr_t)ptr;
   iov[0].iov_len  =          startHeader;
   iov[2].iov_base = (caddr_t)ptr+startHeader;
   iov[2].iov_len  =          post.size()-startHeader;

   method = U_STRING_FROM_CONSTANT("POST");

   if (sendRequest(iov, 6)) U_RETURN(true);

   U_RETURN(false);
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UHttpClient_Base::dump(bool _reset) const
{
   U_CHECK_MEMORY

   UClient_Base::dump(false);

   *UObjectIO::os << '\n'
                  << "bproxy                              " << bproxy                << '\n'
                  << "nResponseCode                       " << nResponseCode         << '\n'
                  << "bFollowRedirects                    " << bFollowRedirects      << '\n'
                  << "body           (UString             " << (void*)&body          << ")\n"
                  << "user           (UString             " << (void*)&user          << ")\n"
                  << "method         (UString             " << (void*)&method        << ")\n"
                  << "password       (UString             " << (void*)&password      << ")\n"
                  << "setcookie      (UString             " << (void*)&setcookie     << ")\n"
                  << "requestHeader  (UMimeHeader         " << (void*)requestHeader  << ")\n"
                  << "responseHeader (UMimeHeader         " << (void*)responseHeader << ')';

   if (_reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
