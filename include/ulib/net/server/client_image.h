// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    client_image.h - Handles accepted connections from UServer's client
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef U_CLIENT_IMAGE_H
#define U_CLIENT_IMAGE_H 1

#include <ulib/event/event_fd.h>
#include <ulib/utility/socket_ext.h>

#ifdef HAVE_SSL
#  include <ulib/ssl/certificate.h>
#  include <ulib/ssl/net/sslsocket.h>
#endif

// #define FILETEST 1

#ifdef U_FILETEST
#  define FILETEST_REQ "request/request.%P"
#  define FILETEST_RES "response/response.%P"
#else
#  define FILETEST_REQ (const char*)0
#  define FILETEST_RES (const char*)0
#endif

/**
   @class UClientImage

   @brief Handles accepted connections from UServer's client
*/

class UHTTP;
class UServer_Base;

class U_EXPORT UClientImage_Base : public UEventFd {
public:

   // NB: these are public for plugin access...

   UString* logbuf;
   UIPAddress* clientAddress;

   // NB: we need that (not put on it in class UClientImage<USSLSocket>) otherwise there are problem with delete[]...

#ifdef HAVE_SSL
   SSL* ssl;
#endif

   static UString* body;
   static USocket* socket;
   static UString* rbuffer;
   static UString* wbuffer;
   static UString* request; // NB: it is only a pointer, not a string object...
   static UString* pbuffer;
   static const char* rpointer;
   static UClientImage_Base* pClientImage;
   static bool bIPv6, pipeline, write_off; // NB: we not send response because we can have used sendfile() etc...
   static uint32_t rstart, size_request, corked;

   // NB: these are for ULib Servlet Page (USP) - U_DYNAMIC_PAGE_OUTPUT...

   static UString* _value;
   static UString* _buffer;
   static UString* _encoded;

   // SERVICES

   int  genericRead();
   bool newConnection();
   void logCertificate(void* x509); // aggiungo nel log il certificato Peer del client ("issuer","serial")

   static void clear();
   static void init(USocket* p);
   static void setTcpCork(uint32_t corked);

   // log

   void logRequest( const char* filereq = FILETEST_REQ);
   void logResponse(const char* fileres = FILETEST_RES);

   // get remote ip address

   static UIPAddress& remoteIPAddress();

   // welcome message

   static void setMsgWelcome(const UString& msg);

   // define method VIRTUAL of class UEventFd

   virtual int  handlerRead();
   virtual int  handlerWrite();
   virtual void handlerDelete();
   virtual void handlerError(int state)
      {
      U_TRACE(0, "UClientImage_Base::handlerError(%d)", state)

      // NB: we need this because we use the same object USocket...

      socket->iState    = state;
      socket->iSockDesc = UEventFd::fd;
      }

   // manage if other request already available... (pipelining)

   static bool isPipeline()
      {
      U_TRACE(0, "UClientImage_Base::isPipeline()")

      U_RETURN(pipeline);
      }

   static void setRequestSize(uint32_t n);

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   static UString* msg_welcome;

   // COSTRUTTORI

            UClientImage_Base();
   virtual ~UClientImage_Base()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UClientImage_Base)

      handlerDelete();
      }

private:
   UClientImage_Base(const UClientImage_Base&) : UEventFd() {}
   UClientImage_Base& operator=(const UClientImage_Base&)   { return *this; }

   friend class UHTTP;
   friend class UServer_Base;
};

template <class Socket> class U_EXPORT UClientImage : public UClientImage_Base {
public:

   // COSTRUTTORI

   UClientImage() : UClientImage_Base()
      {
      U_TRACE_REGISTER_OBJECT(0, UClientImage, "")
      }

   virtual ~UClientImage()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UClientImage)

      U_INTERNAL_DUMP("this = %p", this)
      }

   // DEBUG

#ifdef DEBUG
   const char* dump(bool _reset) const { return UClientImage_Base::dump(_reset); }
#endif

private:
   UClientImage(const UClientImage&) : UClientImage_Base() {}
   UClientImage& operator=(const UClientImage&)            { return *this; }
};

#ifdef HAVE_SSL // specializzazione con USSLSocket

template <> class U_EXPORT UClientImage<USSLSocket> : public UClientImage_Base {
public:

   UClientImage() : UClientImage_Base()
      {
      U_TRACE_REGISTER_OBJECT(0, UClientImage<USSLSocket>, "")

      U_INTERNAL_ASSERT_POINTER(socket)
      U_ASSERT(socket->isSSL())
      }

   virtual ~UClientImage()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UClientImage<USSLSocket>)

      U_INTERNAL_ASSERT_POINTER(socket)
      U_ASSERT(socket->isSSL())
      }

   // SERVICES

   static USSLSocket* getSocket() { return (USSLSocket*)socket; }

   void checkForNewConnection()
      {
      U_TRACE(0, "UClientImage<USSLSocket>::checkForNewConnection()")

      U_INTERNAL_DUMP("fd = %d sock_fd = %d", UEventFd::fd, socket->getFd())

      if (logbuf &&
          UEventFd::fd == 0)
         {
         UClientImage_Base::logCertificate(getSocket()->getPeerCertificate());
         }

      // NB: we need this because we reuse the same object USocket...

      if (ssl == 0)
         {
         ssl = getSocket()->ssl;
               getSocket()->ssl = 0;

         U_INTERNAL_DUMP("ssl = %p ssl_fd = %d", ssl, SSL_get_fd(ssl))
         }
      }

   // define method VIRTUAL of class UEventFd

   virtual int handlerRead()
      {
      U_TRACE(0, "UClientImage<USSLSocket>::handlerRead()")

      checkForNewConnection();

      int result = UClientImage_Base::handlerRead();

      U_RETURN(result);
      }

   virtual void handlerDelete()
      {
      U_TRACE(0, "UClientImage<USSLSocket>::handlerDelete()")

      if (socket->isOpen())
         {
         U_INTERNAL_DUMP("ssl = %p ssl_fd = %d fd = %d sock_fd = %d", ssl, SSL_get_fd(ssl), UEventFd::fd, socket->getFd())

         U_INTERNAL_ASSERT_POINTER(ssl)
         U_INTERNAL_ASSERT_EQUALS(SSL_get_fd(ssl), UEventFd::fd)

         getSocket()->ssl = ssl;
         }

      ssl = 0;

      UClientImage_Base::handlerDelete();
      }

   virtual void handlerError(int state)
      {
      U_TRACE(0, "UClientImage<USSLSocket>::handlerError(%d)", state)

      U_INTERNAL_DUMP("ssl = %p ssl_fd = %d", ssl, SSL_get_fd(ssl))

      U_INTERNAL_ASSERT_POINTER(ssl)
      U_INTERNAL_ASSERT_EQUALS(SSL_get_fd(ssl), UEventFd::fd)

      getSocket()->ssl = ssl; // NB: we need this because we reuse the same object USocket...

      UClientImage_Base::handlerError(state);
      }

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

private:
   UClientImage<USSLSocket>(const UClientImage<USSLSocket>&) : UClientImage_Base() {}
   UClientImage<USSLSocket>& operator=(const UClientImage<USSLSocket>&)            { return *this; }
};

#endif

#endif
