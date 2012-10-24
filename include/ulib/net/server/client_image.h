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

#ifdef USE_LIBSSL
#  include <ulib/ssl/certificate.h>
#  include <ulib/ssl/net/sslsocket.h>
#endif

/*
#define U_FILETEST 1
*/
#ifdef  U_FILETEST
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
class UIPAllow;
class UServer_Base;

class U_EXPORT UClientImage_Base : public UEventFd {
public:

   // NB: these are public for plugin access...

   USocket* socket;
   UString* logbuf;
   time_t last_event;

   static UString* body;
   static UString* rbuffer;
   static UString* wbuffer;
   static UString* request; // NB: it is only a pointer, not a string object...
   static UString* pbuffer;
   static UString* environment;
   static bool bIPv6, pipeline, write_off;
   static uint32_t rstart, size_request, counter;

   // NB: these are for ULib Servlet Page (USP) - USP_PRINTF...

   static UString* _value;
   static UString* _buffer;
   static UString* _encoded;

   // NB: we need that (not put on it in class UClientImage<USSLSocket>) otherwise there are problem with delete[]...

#ifdef USE_LIBSSL
   static SSL_CTX* ctx;
#endif

   // SERVICES

   int  genericRead() __pure;
   void logCertificate(void* x509); // aggiungo nel log il certificato Peer del client ("issuer","serial")

   bool isPendingWrite()
      {
      U_TRACE(0, "UClientImage_Base::isPendingWrite()")

      U_RETURN(count > 0);
      }

   static void init();
   static void clear();
   static void initAfterGenericRead();

   // log

   void logRequest( const char* filereq = FILETEST_REQ);
   void logResponse(const char* fileres = FILETEST_RES);

   // Check whether the ip address client ought to be allowed

   bool isAllowed(UVector<UIPAllow*>& vallow_IP) __pure;

   // welcome message

   static void setMsgWelcome(const UString& msg);

   // define method VIRTUAL of class UEventFd

   virtual int  handlerRead();
   virtual int  handlerWrite();
   virtual void handlerDelete();
   virtual void handlerError(int sock_state);

   // method VIRTUAL to redefine

   virtual bool newConnection();
   virtual int  handlerResponse();

   // manage if other request already available... (pipelining)

   static bool isPipeline()
      {
      U_TRACE(0, "UClientImage_Base::isPipeline()")

      U_RETURN(pipeline);
      }

   static void resetPipeline()
      {
      U_TRACE(0, "UClientImage_Base::resetPipeline()")

      U_INTERNAL_DUMP("pipeline = %b", pipeline)

      if (pipeline)
         {
         U_INTERNAL_ASSERT_POINTER(request)
         U_INTERNAL_ASSERT(request == pbuffer && pbuffer->isNull() == false && pbuffer->same(*rbuffer) == false)

         pbuffer->clear();

         pipeline = false;
         }
      }

   static void manageRequestSize(bool request_resize);

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   uint32_t start, count;
   int state, sfd, bclose;
   const char* client_address;

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

      socket = U_NEW(Socket(UClientImage_Base::bIPv6));

      U_INTERNAL_DUMP("UEventFd::fd = %d", UEventFd::fd)
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

#ifdef USE_LIBSSL // specializzazione con USSLSocket

template <> class U_EXPORT UClientImage<USSLSocket> : public UClientImage_Base {
public:

   UClientImage() : UClientImage_Base()
      {
      U_TRACE_REGISTER_OBJECT(0, UClientImage<USSLSocket>, "")

      socket = U_NEW(USSLSocket(UClientImage_Base::bIPv6, UClientImage_Base::ctx, false));
      }

   virtual ~UClientImage()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UClientImage<USSLSocket>)

      U_INTERNAL_DUMP("this = %p", this)
      }

   // SERVICES

   USSLSocket* getSocket() { return (USSLSocket*)socket; }

   // define method VIRTUAL of class UClientImage_Base

   virtual bool newConnection()
      {
      U_TRACE(0, "UClientImage<USSLSocket>::newConnection()")

      U_INTERNAL_ASSERT_POINTER(socket)
      
      if (logbuf) UClientImage_Base::logCertificate(getSocket()->getPeerCertificate());

      return UClientImage_Base::newConnection();
      }

   // DEBUG

#ifdef DEBUG
   const char* dump(bool _reset) const { return UClientImage_Base::dump(_reset); }
#endif

private:
   UClientImage<USSLSocket>(const UClientImage<USSLSocket>&) : UClientImage_Base() {}
   UClientImage<USSLSocket>& operator=(const UClientImage<USSLSocket>&)            { return *this; }
};

#endif

#endif
