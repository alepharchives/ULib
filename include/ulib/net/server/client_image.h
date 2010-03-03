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
class UTimeoutConnection;

class U_EXPORT UClientImage_Base : public UEventFd {
public:

   // NB: these are public for plugin access...

   UString* logbuf;
   UIPAddress* clientAddress;

   static UString* body;
   static USocket* socket;
   static UString* rbuffer;
   static UString* wbuffer;
   static bool bIPv6, write_off; // NB: we not send response because we can have used sendfile() etc...
   static UClientImage_Base* pClientImage;

   // NB: these are for ULib Servlet Page (USP) - U_DYNAMIC_PAGE_OUTPUT...

   static UString* _value;
   static UString* _buffer;
   static UString* _encoded;

   // SERVICES

   static void clear();
   static void run();
   static void resetBuffer();
   static void genericReset();
   static void init(USocket* p);

   // check if data read already available... (pipelining)

   static bool isPipeline(uint32_t size)
      {
      U_TRACE(0, "UClientImage_Base::isPipeline(%u)", size)

      U_INTERNAL_DUMP("size_message = %u pcount = %d pbuffer = %p", USocketExt::size_message, USocketExt::pcount, USocketExt::pbuffer)

      if ((int32_t)USocketExt::size_message  > 0 && size &&
          (USocketExt::pcount                > 0 || size > USocketExt::size_message))
         {
         U_RETURN(true);
         }

      U_RETURN(false);
      }

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

   // check for X-Forwarded-For: client1, proxy1, proxy2

   static const char* getRemoteInfo(uint32_t* plen);

   // welcome message

   static void setMsgWelcome(const UString& msg);

   // define method VIRTUAL to redefine

   virtual void reset()
      {
      U_TRACE(0, "UClientImage_Base::reset()")

      reuse(); // NB: we need this because we reuse the same object USocket...

      genericReset();
      }

   // define method VIRTUAL of class UEventFd

   virtual int handlerRead();
   virtual int handlerWrite();

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   static UString* msg_welcome;

   // NB: we need this because we reuse the same object USocket...

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

      reuse(); // NB: we need this because we reuse the same object USocket...

      destroy();
      }

   static void destroy();

   // check if data read already available... (pipelining)

   static bool isPipeline();

   static void checkForPipeline()
      {
      U_TRACE(0, "UClientImage_Base::checkForPipeline()")

      U_INTERNAL_ASSERT_POINTER(rbuffer)

      uint32_t size = rbuffer->size();

      U_INTERNAL_DUMP("rbuffer->size() = %u size_message = %u pcount = %d pbuffer = %p",
                       size, USocketExt::size_message, USocketExt::pcount, USocketExt::pbuffer)

      U_INTERNAL_ASSERT_MAJOR((int32_t)USocketExt::size_message,0)

      if (size > USocketExt::size_message)
         {
         USocketExt::pcount  = size - USocketExt::size_message;
#     ifdef DEBUG
         USocketExt::pbuffer = rbuffer->data();
#     endif

         U_INTERNAL_DUMP("pcount = %d pbuffer = %p", USocketExt::pcount, USocketExt::pbuffer)
         }
      }

private:
   UClientImage_Base(const UClientImage_Base&) : UEventFd() {}
   UClientImage_Base& operator=(const UClientImage_Base&)   { return *this; }

   friend class UHTTP;
   friend class UServer_Base;
   friend class UTimeoutConnection;
};

template <class Socket> class U_EXPORT UClientImage : public UClientImage_Base {
public:

   // COSTRUTTORI

   UClientImage() : UClientImage_Base()
      {
      U_TRACE_REGISTER_OBJECT(0, UClientImage, "", 0)
      }

   virtual ~UClientImage()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UClientImage)
      }

   static Socket* getSocket() { return (Socket*) socket; }

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const { return UClientImage_Base::dump(reset); }
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
      U_TRACE_REGISTER_OBJECT(0, UClientImage<USSLSocket>, "", 0)

      // NB: we need this because we reuse the same object USocket...

      ssl = getSocket()->ssl;

      if (logbuf) logCertificate();
      }

   virtual ~UClientImage()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UClientImage<USSLSocket>)

      U_INTERNAL_ASSERT_POINTER(socket)
      U_INTERNAL_ASSERT_EQUALS(socket->isSSL(), true)

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

   // aggiungo nel log il certificato Peer del client ("issuer","serial")

   void logCertificate()
      {
      U_TRACE(0, "UClientImage<USSLSocket>::logCertificate()")

      // NB: OpenSSL already tested the cert validity during SSL handshake and returns a X509 ptr just if the certificate is valid...

      UClientImage_Base::logCertificate(getSocket()->getPeerCertificate());
      }

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
