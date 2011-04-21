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

   static UString* body;
   static USocket* socket;
   static UString* rbuffer;
   static UString* wbuffer;
   static UString* request; // NB: it is only a pointer, not a string object...
   static UString* pbuffer;
   static const char* rpointer;
   static uint32_t rstart, size_request;
   static UClientImage_Base* pClientImage;
   static bool bIPv6, pipeline, write_off; // NB: we not send response because we can have used sendfile() etc...

   // NB: these are for ULib Servlet Page (USP) - U_DYNAMIC_PAGE_OUTPUT...

   static UString* _value;
   static UString* _buffer;
   static UString* _encoded;

   // SERVICES

   static void clear();
   static int  genericRead();
   static void init(USocket* p);

   // log

   static void logRequest( const char* filereq = FILETEST_REQ);
   static void logResponse(const char* fileres = FILETEST_RES);

   static void logCertificate(void* x509); // aggiungo nel log il certificato Peer del client ("issuer","serial")

   // get remote ip address

   static UIPAddress& remoteIPAddress()
      {
      U_TRACE(0, "UClientImage_Base::remoteIPAddress()")

      U_INTERNAL_ASSERT_POINTER(socket)
      U_INTERNAL_ASSERT_POINTER(pClientImage)

      if (pClientImage->logbuf) return *pClientImage->clientAddress;

      socket->setRemote();

      return socket->remoteIPAddress();
      }

   // welcome message

   static void setMsgWelcome(const UString& msg);

   // define method VIRTUAL to redefine

   virtual void reset();

   // define method VIRTUAL of class UEventFd

   virtual int handlerRead();
   virtual int handlerWrite();
   virtual int handlerError()
      {
      U_TRACE(0, "UClientImage_Base::handlerError()")

      resetSocket(USocket::RESET); // NB: we need this because we reuse the same object USocket...

      U_RETURN(U_NOTIFIER_DELETE);
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

   // NB: we need this because we reuse the same object UClientImage_Base...

   void reuse()
      {
      U_TRACE(0, "UClientImage_Base::reuse()")

      pClientImage = this;

      U_INTERNAL_DUMP("pClientImage = %p fd = %d socket->iSockDesc = %d", pClientImage, UEventFd::fd, socket->iSockDesc)
      }

   void resetSocket(USocket::State state)
      {
      U_TRACE(0, "UClientImage_Base::resetSocket(%d)", state)

      U_INTERNAL_DUMP("pClientImage = %p fd = %d socket->iSockDesc = %d", pClientImage, UEventFd::fd, socket->iSockDesc)

      socket->iState    = state;
      socket->iSockDesc = UEventFd::fd;
      }

   // COSTRUTTORI

            UClientImage_Base();
   virtual ~UClientImage_Base()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UClientImage_Base)

      reuse(); // NB: we need this because we reuse the same object UClientImage_Base...

      destroy();
      }

   static void destroy();

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
      }

   static Socket* getSocket() { return (Socket*) socket; }

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

   SSL* ssl;

   UClientImage() : UClientImage_Base()
      {
      U_TRACE_REGISTER_OBJECT(0, UClientImage<USSLSocket>, "")

      // NB: we need this because we reuse the same object USocket...

      ssl = getSocket()->ssl;

      if (logbuf) UClientImage_Base::logCertificate(getSocket()->getPeerCertificate());
      }

   virtual ~UClientImage()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UClientImage<USSLSocket>)

      U_INTERNAL_ASSERT_POINTER(socket)
      U_INTERNAL_ASSERT(socket->isSSL())

      if (socket->isOpen())
         {
         U_INTERNAL_DUMP("ssl = %p ssl_fd = %d fd = %d sock_fd = %d", ssl, SSL_get_fd(ssl), UEventFd::fd, socket->getFd())

         U_INTERNAL_ASSERT_EQUALS(SSL_get_fd(ssl), UEventFd::fd)

         // NB: we need this because we reuse the same object USocket...

         getSocket()->ssl = ssl;
         }
      }

   // SERVICES

   static USSLSocket* getSocket() { return (USSLSocket*)socket; }

   // define method VIRTUAL of class UClientImage_Base

   virtual void reset()
      {
      U_TRACE(0, "UClientImage<USSLSocket>::reset()")

      U_INTERNAL_DUMP("ssl = %p ssl_fd = %d fd = %d", ssl, SSL_get_fd(ssl), UEventFd::fd)

      U_INTERNAL_ASSERT(UEventFd::fd == SSL_get_fd(ssl))

      getSocket()->ssl = ssl; // NB: we need this because we reuse the same object USocket...

      UClientImage_Base::reset();
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
