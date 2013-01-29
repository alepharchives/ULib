// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    client.h - manages a client connection with a server
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef U_CLIENT_H
#define U_CLIENT_H 1

#include <ulib/url.h>
#include <ulib/net/rpc/rpc.h>
#include <ulib/utility/uhttp.h>

#ifdef USE_LIBSSL
#  include <ulib/ssl/certificate.h>
#  include <ulib/ssl/net/sslsocket.h>
#endif

/**
   @class UClient

   @brief Handles a connections with a server
*/

class ULog;
class UFileConfig;
class UFCGIPlugIn;
class USCGIPlugIn;
class UProxyPlugIn;
class UHttpClient_Base;

class U_EXPORT UClient_Base {
public:

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   // SERVICES

   static const UString* str_RES_TIMEOUT;

   static void str_allocate();

   bool isClosed() const
      {
      U_TRACE(0, "UClient_Base::isClosed()")

      bool result = socket->isClosed();

      U_RETURN(result);
      }

   bool isConnected() const
      {
      U_TRACE(0, "UClient_Base::isConnected()")

      bool result = socket->isConnected();

      U_RETURN(result);
      }

   void reset()
      {
      U_TRACE(0, "UClient_Base::reset()")

#  ifdef DEBUG
      uri.clear(); // NB: to avoid DEAD OF SOURCE STRING WITH CHILD ALIVE... (uri can be a substr of url)
#  endif
      url.clear();
      }

   void reserve(uint32_t n)
      {
      U_TRACE(0, "UClient_Base::reserve(%u)", n)

      (void) response.reserve(n);
      }

   void close()
      {
      U_TRACE(0, "UClient_Base::close()")

      U_INTERNAL_ASSERT_POINTER(socket)

      socket->close();
      }

   bool shutdown(int how = SHUT_WR)
      {
      U_TRACE(0, "UClient_Base::shutdown(%d)", how)

      U_INTERNAL_ASSERT_POINTER(socket)

      bool result = socket->shutdown(how);

      U_RETURN(result);
      }

   void setTimeOut(uint32_t t) { timeoutMS = t; }

   // LOG 

   static void setLogShared();

   static bool isLogSharedWithServer()
      {
      U_TRACE(0, "UClient_Base::isLogSharedWithServer()")

      U_RETURN(log_shared_with_server);
      }

   int         getPort() const         { return port; }
   UString     getServer() const       { return server; }
   UString     getBuffer() const       { return buffer; }
   UString     getResponse() const     { return response; }
   const char* getResponseData() const { return response.data(); }

   void logResponse(const UString& data);
   void loadConfigParam(UFileConfig& file);
   bool setHostPort(const UString& host, int port);

   bool connect();
   void clearData();
   bool readHTTPResponse();
   bool readResponse(int count = U_SINGLE_READ);
   bool sendRequest(struct iovec* iov, int iovcnt, bool bread_response);

   bool sendRequest(const UString& req, bool bread_response)
      {
      U_TRACE(0, "UClient_Base::sendRequest(%.*S,%b)", U_STRING_TO_TRACE(req), bread_response)

      struct iovec iov[1] = { { (caddr_t)req.data(), req.size() } };

      bool result = sendRequest(iov, 1, bread_response);

      U_RETURN(result);
      }

   // Add the MIME-type headers to the request for HTTP server

   UString wrapRequestWithHTTP(UString* req,
                               const char* method, uint32_t method_len,
                               const char* _uri,   uint32_t uri_len,
                               const char* extension, const char* content_type = 0);

   // -----------------------------------------------------------------------------------------------------------------------
   // Very simple RPC-like layer
   //
   // Requests and responses are build of little packets each containing a 4-byte ascii token, an 8-byte hex value or length,
   // and optionally data corresponding to the length.
   // -----------------------------------------------------------------------------------------------------------------------

   bool readRPCResponse();

   // Transmit token name (4 characters) and value (32-bit int, as 8 hex characters)

   bool sendTokenInt(const char* token, uint32_t value)
      { buffer.setEmpty(); return URPC::sendTokenInt(socket, token, value, buffer); }

   // Write a token, and then the string data

   bool sendTokenString(const char* token, const UString& data)
      { buffer.setEmpty(); return URPC::sendTokenString(socket, token, data, buffer); }

   // Transmit an vector of string

   bool sendTokenVector(const char* token, UVector<UString>& vec)
      { buffer.setEmpty(); return URPC::sendTokenVector(socket, token, vec, buffer); }

   // DEBUG

#ifdef DEBUG
   const char* dump(bool _reset) const;
#endif

protected:
   USocket* socket;
   UString server,      // host name or ip address for server
           cert_file,   // locations for certificate of client
           key_file,    // locations for private key of client
           password,    // password  for private key of client
           ca_file,     // locations of trusted CA certificates used in the verification
           ca_path,     // locations of trusted CA certificates used in the verification
           log_file,    // locations for file log
           uri,
           response,
           buffer,
           host_port,
           logbuf;
   Url url;
   int port,            // the port number to connect to
       timeoutMS,       // the time-out value in milliseconds
       verify_mode;     // mode of verification of connection
   bool bIPv6;

   static ULog* log;
   static bool  log_shared_with_server;

   bool setUrl(const UString& url);

   // COSTRUTTORI

    UClient_Base();
    UClient_Base(UFileConfig* cfg);
   ~UClient_Base();

private:
   UClient_Base(const UClient_Base&)            {}
   UClient_Base& operator=(const UClient_Base&) { return *this; }

   friend class UFCGIPlugIn;
   friend class USCGIPlugIn;
   friend class UProxyPlugIn;
   friend class UHttpClient_Base;
};

template <class Socket> class U_EXPORT UClient : virtual public UClient_Base {
public:

   // COSTRUTTORI

   UClient(UFileConfig* cfg) : UClient_Base(cfg)
      {
      U_TRACE_REGISTER_OBJECT(0, UClient, "%p", cfg)

      socket = U_NEW(Socket(bIPv6));
      }

   ~UClient()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UClient)
      }

   Socket* getSocket() { return (Socket*) socket; }

   // DEBUG

#ifdef DEBUG
   const char* dump(bool _reset) const { return UClient_Base::dump(_reset); }
#endif

private:
   UClient(const UClient&) : UClient_Base(0) {}
   UClient& operator=(const UClient&)        { return *this; }
};

#ifdef USE_LIBSSL // specializzazione con USSLSocket

template <> class U_EXPORT UClient<USSLSocket> : virtual public UClient_Base {
public:

   UClient(UFileConfig* cfg) : UClient_Base(cfg)
      {
      U_TRACE_REGISTER_OBJECT(0, UClient<USSLSocket>, "%p", cfg)

      U_INTERNAL_ASSERT(ca_file.isNullTerminated())
      U_INTERNAL_ASSERT(ca_path.isNullTerminated())
      U_INTERNAL_ASSERT(key_file.isNullTerminated())
      U_INTERNAL_ASSERT(password.isNullTerminated())
      U_INTERNAL_ASSERT(cert_file.isNullTerminated())

      socket = U_NEW(USSLSocket(bIPv6, 0, false));

      // Load our certificate

      if (getSocket()->setContext(0, cert_file.data(), key_file.data(), password.data(), ca_file.data(),  ca_path.data(), verify_mode) == false)
         {
         U_ERROR("SSL setContext() failed...");
         }
      }

   ~UClient()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UClient<USSLSocket>)
      }

   USSLSocket* getSocket() { return (USSLSocket*) socket; }

   // DEBUG

#ifdef DEBUG
   const char* dump(bool _reset) const { return UClient_Base::dump(_reset); }
#endif

private:
   UClient<USSLSocket>(const UClient<USSLSocket>&) : UClient_Base(0) {}
   UClient<USSLSocket>& operator=(const UClient<USSLSocket>&)        { return *this; }
};

#endif

#endif
