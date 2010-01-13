// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    mod_proxy.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef U_MOD_PROXY_H
#define U_MOD_PROXY_H 1

#include <ulib/net/client/http.h>
#include <ulib/net/server/server_plugin.h>

/*
The plugin interface is an integral part of UServer which provides a flexible way to add specific functionality to UServer.
Plugins allow you to enhance the functionality of UServer without changing the core of the server. They can be loaded at
startup time and can change virtually some aspect of the behaviour of the server.

UServer has 5 hooks which are used in different states of the execution of the request:
--------------------------------------------------------------------------------------------
* Server-wide hooks:
````````````````````
1) handlerConfig: called when the server finished to process its configuration
2) handlerInit:   called when the server finished its init, and before start to run

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
  U_PLUGIN_HANDLER_AGAIN    if the request is empty (NONBLOCKING)

  U_PLUGIN_HANDLER_ERROR    on error
*/

class UModProxyService;

class U_EXPORT UProxyPlugIn : public UServerPlugIn {
public:

   // COSTRUTTORI

   UProxyPlugIn() : client_http((UFileConfig*)0)
      {
      U_TRACE_REGISTER_OBJECT(0, UProxyPlugIn, "", 0)
      }

   virtual ~UProxyPlugIn()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UProxyPlugIn)
      }

   // define method VIRTUAL of class UServerPlugIn

   // Server-wide hooks

   virtual int handlerConfig(UFileConfig& cfg);
   virtual int handlerInit();

   // Connection-wide hooks

   virtual int handlerRead()
      {
      U_TRACE(0, "UProxyPlugIn::handlerRead()")

      U_RETURN(U_PLUGIN_HANDLER_GO_ON);
      }

   virtual int handlerRequest();
   virtual int handlerReset();

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   UVector<UString> vmsg_error;
   UHttpClient<USocket> client_http;
   UVector<UModProxyService*> vservice;
   bool is_connect;

private:
   UProxyPlugIn(const UProxyPlugIn&) : UServerPlugIn(), client_http((UFileConfig*)0) {}
   UProxyPlugIn& operator=(const UProxyPlugIn&)                                      { return *this; }
};

#endif
