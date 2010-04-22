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

#ifdef HAVE_SSL
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

class U_EXPORT UClient_Base {
public:

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   // SERVICES

   static UString* str_RES_TIMEOUT;

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

   bool isHttps() const
      {
      U_TRACE(0, "UClient_Base::isHttps()")

      if (Url::str_https == 0) Url::str_allocate();

      bool result = (UClient_Base::service == *Url::str_https);

      U_RETURN(result);
      }

   void reset()
      {
      U_TRACE(0, "UClient_Base::reset()")

      request.clear();
      }

   void close()
      {
      U_TRACE(0, "UClient_Base::close()")

      U_INTERNAL_ASSERT_POINTER(socket)

      socket->close();
      }

   int         getPort() const         { return port; }
   UString     getServer() const       { return server; }
   UString     getBuffer() const       { return buffer; }
   UString     getResponse() const     { return response; }
   const char* getResponseData() const { return response.data(); }

   void logResponse(const UString& data);
   void loadConfigParam(UFileConfig& file);
   bool setHostPort(const UString& host, int port);

   void setTimeOut(uint32_t t) { timeoutMS = t; }

   // Add the MIME-type headers to the request for HTTP server

   void wrapRequestWithHTTP(const char* extension, const char* content_type = 0);

   // method to redefine

   bool connect();
   void clearData();
   bool readHTTPResponse();
   bool readResponse(int count = U_SINGLE_READ);

   bool sendRequest();
   bool sendRequest(const UString& data)            { request =         data;       return sendRequest(); }
   bool sendRequest(const char* data, uint32_t len) { request = UString(data, len); return sendRequest(); }

   // -----------------------------------------------------------------------------------------------------------------------
   // Very simple RPC-like layer
   //
   // Requests and responses are build of little packets each containing a 4-byte ascii token, an 8-byte hex value or length,
   // and optionally data corresponding to the length.
   // -----------------------------------------------------------------------------------------------------------------------

   // Transmit token name (4 characters) and value (32-bit int, as 8 hex characters)

   bool sendTokenInt(const char* token, uint32_t value)
      { buffer.setEmpty(); return URPC::sendTokenInt(socket, token, value, buffer); }

   // Write a token, and then the string data

   bool sendTokenString(const char* token, const UString& data)
      { buffer.setEmpty(); return URPC::sendTokenString(socket, token, data, buffer); }

   // Transmit an vector of string

   bool sendTokenVector(const char* token, UVector<UString>& vec)
      { buffer.setEmpty(); return URPC::sendTokenVector(socket, token, vec, buffer); }

   // Read a token and value

   bool readTokenInt(const char* token)
      { response.setEmpty(); return (URPC::readTokenInt(socket, token, response) > 0); }

   // Read a token, and then the string data

   bool readTokenString(const char* token)
      {
      U_TRACE(0, "UClient_Base::readTokenString(%S)", token)

      // NB: we force for U_SUBSTR_INC_REF case (string can be referenced more)...

        buffer.setEmptyForce();
      response.setEmptyForce();

      if (URPC::readTokenString(socket, token, buffer, response))
         {
         // NB: we force for U_SUBSTR_INC_REF case (string can be referenced more)...

         buffer.size_adjust_force(U_TOKEN_NM);

         U_INTERNAL_DUMP("buffer = %.*S response = %.*S)", U_STRING_TO_TRACE(buffer), U_STRING_TO_TRACE(response))

         U_RETURN(true);
         }

      U_RETURN(false);
      }

   bool readRPCResponse()
      {
      U_TRACE(0, "UClient_Base::readRPCResponse()")

      bool result = (this->readTokenString(0) || this->UClient_Base::buffer.empty() == false); 

      U_RETURN(result);
      }

   // Read an vector of string from the network

   bool readTokenVector(const char* token, UVector<UString>& vec)
      { response.setEmpty(); return URPC::readTokenVector(socket, token, response, vec); }

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   USocket* socket;

   int port,            // the port number to connect to
       verify_mode;     // mode of verification of connection
   uint32_t timeoutMS;  // the time-out value in milliseconds. A zero value indicates that the client will wait forever,
                        // or until the underlying operating system decides that the connection cannot be established

   UString server,      // host name or ip address for server
           cert_file,   // locations for certificate of client
           key_file,    // locations for private key of client
           password,    // password  for private key of client
           ca_file,     // locations of trusted CA certificates used in the verification
           ca_path,     // locations of trusted CA certificates used in the verification
           log_file,    // locations for file log
           request,
           response,
           buffer,
           service,
           host_port,
           logbuf;

   bool bIPv6;

   static ULog* log;
   static bool log_shared_with_server;

   bool setUrl(const UString& newLocation);

   // COSTRUTTORI

    UClient_Base(UFileConfig* cfg);
   ~UClient_Base();

private:
   UClient_Base(const UClient_Base&)            {}
   UClient_Base& operator=(const UClient_Base&) { return *this; }

   friend class UFCGIPlugIn;
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
   const char* dump(bool reset) const { return UClient_Base::dump(reset); }
#endif

private:
   UClient(const UClient&) : UClient_Base(0) {}
   UClient& operator=(const UClient&)        { return *this; }
};

#ifdef HAVE_SSL // specializzazione con USSLSocket

template <> class U_EXPORT UClient<USSLSocket> : virtual public UClient_Base {
public:

   UClient(UFileConfig* cfg) : UClient_Base(cfg)
      {
      U_TRACE_REGISTER_OBJECT(0, UClient<USSLSocket>, "%p", cfg)

      USSLSocket::method = (SSL_METHOD*) SSLv3_client_method();

      socket = U_NEW(USSLSocket(bIPv6));

      // These are the 1024 bit DH parameters from "Assigned Number for SKIP Protocols"
      // (http://www.skip-vpn.org/spec/numbers.html).
      // See there for how they were generated.

      getSocket()->useDHFile();

      // Load our certificate

      if (getSocket()->setContext(cert_file.c_str(), key_file.c_str(), password.c_str(),
                                    ca_file.c_str(),  ca_path.c_str(), verify_mode) == false)
         {
         U_ERROR("SSL setContext() failed...", 0);
         }
      }

   ~UClient()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UClient<USSLSocket>)
      }

   USSLSocket* getSocket() { return (USSLSocket*) socket; }

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const { return UClient_Base::dump(reset); }
#endif

private:
   UClient<USSLSocket>(const UClient<USSLSocket>&) : UClient_Base(0) {}
   UClient<USSLSocket>& operator=(const UClient<USSLSocket>&)        { return *this; }
};

#endif

#endif
