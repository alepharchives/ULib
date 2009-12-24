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
3) handlerRead:
4) handlerRequest:
5) handlerReset:
  called in `UClientImage_Base::handlerRead()`
--------------------------------------------------------------------------------------------

RETURNS:
  U_PLUGIN_HANDLER_GO_ON    if ok
  U_PLUGIN_HANDLER_FINISHED if the final output is prepared

  U_PLUGIN_HANDLER_ERROR    on error
*/

class U_EXPORT UHttpPlugIn : public UServerPlugIn {
public:

   static UString* str_URI_PROTECTED_MASK;
   static UString* str_URI_PROTECTED_ALLOWED_IP;

   static void str_allocate();

   // COSTRUTTORI

   UHttpPlugIn() : alias(U_CAPACITY), environment(U_CAPACITY)
      {
      U_TRACE_REGISTER_OBJECT(0, UHttpPlugIn, "", 0)

      index_alias = U_NOT_FOUND;

      if (str_URI_PROTECTED_MASK == 0) str_allocate();
      }

   virtual ~UHttpPlugIn()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UHttpPlugIn)

      if (vallow_IP) delete vallow_IP;
      }

   // define method VIRTUAL of class UServerPlugIn

   // Server-wide hooks

   virtual int handlerConfig(UFileConfig& cfg);
   virtual int handlerInit();

   // Connection-wide hooks

   virtual int handlerRead();
   virtual int handlerRequest();
   virtual int handlerReset()
      {
      U_TRACE(0, "UHttpPlugIn::handlerReset()")

      U_RETURN(U_PLUGIN_HANDLER_GO_ON);
      }

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   uint32_t index_alias;
   UVector<UString> valias;
   UString alias, environment, uri_protected_mask, uri_protected_allowed_ip;

   static bool virtual_host;
   static UVector<UIPAllow*>* vallow_IP;

   UString* getCGIEnvironment();

private:
   UHttpPlugIn(const UHttpPlugIn&) : UServerPlugIn() {}
   UHttpPlugIn& operator=(const UHttpPlugIn&)        { return *this; }
};

#endif
