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
#include <ulib/container/vector.h>
#include <ulib/net/server/server_plugin.h>

/*
The plugin interface is an integral part of UServer which provides a flexible way to add specific functionality to UServer.
Plugins allow you to enhance the functionality of UServer without changing the core of the server. They can be loaded at
startup time and can change virtually some aspect of the behaviour of the server.

UServer has 5 hooks which are used in different states of the execution of the request:
--------------------------------------------------------------------------------------------
* Server-wide hooks:
````````````````````
1) handlerConfig:  called when the server finished to process its configuration
2) handlerInit:    called when the server finished its init, and before start to run

* Connection-wide hooks:
````````````````````````
3) handlerREAD:
4) handlerRequest:
5) handlerReset:
  called in `UClientImage_Base::handlerRead()`
--------------------------------------------------------------------------------------------

RETURNS:
  U_PLUGIN_HANDLER_GO_ON    if ok
  U_PLUGIN_HANDLER_FINISHED if the final output is prepared
  U_PLUGIN_HANDLER_AGAIN    if the request is empty (NONBLOCKING)

  U_PLUGIN_HANDLER_ERROR    on error
*/

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
   static const UString* str_STRICT_TRANSPORT_SECURITY;
   static const UString* str_SESSION_COOKIE_OPTION;
   static const UString* str_MAINTENANCE_MODE;

   static void str_allocate();

   // COSTRUTTORI

   UHttpPlugIn()
      {
      U_TRACE_REGISTER_OBJECT(0, UHttpPlugIn, "")

      valias                = 0;
      maintenance_mode_page = 0;
      uri_request_cert_mask = 0;

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
   const char* dump(bool reset) const;
#endif

protected:
   UVector<UString>* valias;
   UString* maintenance_mode_page;
   UString* uri_request_cert_mask;
   UString uri_protected_allowed_ip;

private:
   UHttpPlugIn(const UHttpPlugIn&) : UServerPlugIn(), UEventFd() {}
   UHttpPlugIn& operator=(const UHttpPlugIn&)                    { return *this; }
};

#endif
