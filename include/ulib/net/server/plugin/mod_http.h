// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    mod_http.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef U_MOD_HTTP_H
#define U_MOD_HTTP_H 1

#include <ulib/event/event_fd.h>
#include <ulib/net/server/server_plugin.h>

class U_EXPORT UHttpPlugIn : public UServerPlugIn, UEventFd {
public:

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   static const UString* str_CACHE_FILE_MASK;
   static const UString* str_URI_PROTECTED_MASK;
   static const UString* str_URI_REQUEST_CERT_MASK;
   static const UString* str_URI_PROTECTED_ALLOWED_IP;
   static const UString* str_LIMIT_REQUEST_BODY;
   static const UString* str_REQUEST_READ_TIMEOUT;
   static const UString* str_ENABLE_INOTIFY;
   static const UString* str_ENABLE_CACHING_BY_PROXY_SERVERS;
   static const UString* str_TELNET_ENABLE;
   static const UString* str_MIN_SIZE_FOR_SENDFILE;
   static const UString* str_URI_REQUEST_STRICT_TRANSPORT_SECURITY_MASK;
   static const UString* str_SESSION_COOKIE_OPTION;
   static const UString* str_MAINTENANCE_MODE;
   static const UString* str_APACHE_LIKE_LOG;

   static void str_allocate();

   // COSTRUTTORI

   UHttpPlugIn()
      {
      U_TRACE_REGISTER_OBJECT(0, UHttpPlugIn, "")

      if (str_URI_PROTECTED_MASK == 0) str_allocate();
      }

   virtual ~UHttpPlugIn();

   // define method VIRTUAL of class UServerPlugIn

   // Server-wide hooks

   virtual int handlerConfig(UFileConfig& cfg);
   virtual int handlerInit();

   // Connection-wide hooks

   virtual int handlerREAD();
   virtual int handlerRequest();

   // define method VIRTUAL of class UEventFd

   virtual int  handlerRead();
   virtual void handlerDelete()
      {
      U_TRACE(0, "UHttpPlugIn::handlerDelete()")

      U_INTERNAL_DUMP("UEventFd::fd = %d", UEventFd::fd)

      UEventFd::fd = 0;
      }

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const { return UEventFd::dump(reset); }
#endif

private:
   UHttpPlugIn(const UHttpPlugIn&) : UServerPlugIn(), UEventFd() {}
   UHttpPlugIn& operator=(const UHttpPlugIn&)                    { return *this; }
};

#endif
