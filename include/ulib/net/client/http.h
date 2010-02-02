// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    http.h - client HTTP
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef U_HTTP_CLIENT_H
#define U_HTTP_CLIENT_H 1

#include <ulib/mime/entity.h>
#include <ulib/net/client/client.h>

/**
   @class UHttpClient
   
   @brief Creates and manages a client connection with a HTTP server.

   <p>HTTP stands for Hyper Text Transfer Protocol, which is defined by
   RFCs published by the <a href="http://www.ietf.org/">Internet Engineering
   Task Force</a>.</p>

   A UHttpClient instance can make only one HTTP request (although
   this one request may involve several message exchanges if HTTP redirection
   or authorization is involved). However, HTTP 1.1 introduces
   persistent connections which can make more effective use of TCP/IP.
*/

class U_EXPORT UHttpClient_Base : virtual public UClient_Base {
public:

   static UString* str_www_authenticate;
   static UString* str_proxy_authenticate;
   static UString* str_proxy_authorization;

   static void str_allocate();

   void reset();
   void setHostForbidden(const UString& host) { forbidden = host; }

   // Returns a modifiable sequence of MIME-type headers that will be used to form a request to the HTTP server

   UMimeHeader* getRequestHeader() { return requestHeader; }

   /**
   Sets a request MIME header value. If a MIME header with the specified
   name already exists, its value is replaced with the supplied value

   @param name the name by which the property is known
   @param value the value to be associated with the named property
   */

   void setHeader(const UString& name, const UString& value) { requestHeader->setHeader(name, value); }

   void setHeaderHostPort(const UString& h)  { setHeader(*USocket::str_host, h); }
   void setHeaderUserAgent(const UString& u) { setHeader(*USocket::str_user_agent, u); }

   void removeHeader(const UString& name) { requestHeader->removeHeader(name); }

   // Returns the MIME header that were received in response from the HTTP server

   UMimeHeader* getResponseHeader() { return responseHeader; }

   /**
   Sets a flag indicating if HTTP redirects will be followed.

   @param bFollow @c true for redirect requests to be followed (the default);
                  @c false prevents this behaviour
   */

   bool getFollowRedirects() const        { return bFollowRedirects; }
   void setFollowRedirects(bool bFollow)  { bFollowRedirects = bFollow; }

   // In response to a HTTP_UNAUTHORISED response from the HTTP server,
   // obtain a userid and password for the scheme/realm returned from the HTTP server

   void setRequestPasswordAuthentication(const UString& _user, const UString& _password) { user = _user; password = _password; }

   /**
   Establishes a TCP/IP socket connection with the host that will satisfy requests for the provided URL.
   This may connect to the host name contained within the URL, or to a proxy server if one has been set.
   This function does not send any information to the remote server after the connection is established

   @param url a fully-qualified http URL for the required resource
   @sa sendRequest()
   */

   bool connectServer(Url& location)
      {
      U_TRACE(0, "UHttpClient_Base::connectServer(%p)", &location)

      if (UClient_Base::setUrl(location.get()) && UClient_Base::isConnected()) UClient_Base::socket->close(); // change server and/or port to connect...

      return UClient_Base::connect();
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

   bool sendRequest(UString& data);
   bool upload(Url& location, UFile& file);
   bool sendPost(Url& location, const UString& pbody, const char* content_type = "application/x-www-form-urlencoded");

   UString getContent() const   { return body; }
   int     responseCode() const { return UHTTP::http_info.nResponseCode; }

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   UMimeHeader* requestHeader;
   UMimeHeader* responseHeader;
   UString body, forbidden, user, password, method, uri;
   bool bFollowRedirects;

    UHttpClient_Base(UFileConfig* cfg);
   ~UHttpClient_Base();

   void composeRequest(UString& data, uint32_t& startHeader);

   // In response to a HTTP_UNAUTHORISED response from the HTTP server, this function will attempt
   // to generate an Authentication header to satisfy the server.

   bool createAuthorizationHeader();
   int  checkResponse(int& redirectCount);

private:
   UHttpClient_Base(const UHttpClient_Base&) : UClient_Base(0) {}
   UHttpClient_Base& operator=(const UHttpClient_Base&)        { return *this; }
};

template <class Socket> class U_EXPORT UHttpClient : public UHttpClient_Base,  public UClient<Socket> {
public:

   // COSTRUTTORI

   UHttpClient(UFileConfig* cfg) : UClient_Base(cfg), UHttpClient_Base(cfg), UClient<Socket>(cfg)
      {
      U_TRACE_REGISTER_OBJECT(0, UHttpClient, "%p", cfg)
      }

   ~UHttpClient()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UHttpClient)
      }

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const { return UHttpClient_Base::dump(reset); }
#endif

private:
   UHttpClient(const UHttpClient&) : UClient_Base(0), UHttpClient_Base(0), UClient<Socket>(0) {}
   UHttpClient& operator=(const UHttpClient&)                                                 { return *this; }
};

#ifdef HAVE_SSL // specializzazione con USSLSocket

template <> class U_EXPORT UHttpClient<USSLSocket> : public UHttpClient_Base,  public UClient<USSLSocket> {
public:

   UHttpClient(UFileConfig* cfg) : UClient_Base(cfg), UHttpClient_Base(cfg), UClient<USSLSocket>(cfg)
      {
      U_TRACE_REGISTER_OBJECT(0, UHttpClient<USSLSocket>, "%p", cfg)
      }

   ~UHttpClient()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UHttpClient<USSLSocket>)
      }

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const { return UHttpClient_Base::dump(reset); }
#endif

protected:

private:
   UHttpClient<USSLSocket>(const UHttpClient<USSLSocket>&) : UClient_Base(0), UHttpClient_Base(0), UClient<USSLSocket>(0) {}
   UHttpClient<USSLSocket>& operator=(const UHttpClient<USSLSocket>&)                                                     { return *this; }
};

#endif

#endif
