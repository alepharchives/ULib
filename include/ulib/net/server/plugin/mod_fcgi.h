// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    mod_fcgi.h - Fast CGI
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef U_MOD_FCGI_H
#define U_MOD_FCGI_H 1

#include <ulib/string.h>
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

class UClient_Base;

class U_EXPORT UFCGIPlugIn : public UServerPlugIn {
public:

   static const UString* str_FCGI_URI_MASK;
   static const UString* str_FCGI_KEEP_CONN;

   static void str_allocate();

   // COSTRUTTORI

   UFCGIPlugIn()
      {
      U_TRACE_REGISTER_OBJECT(0, UFCGIPlugIn, "")

      connection = 0;

      if (str_FCGI_URI_MASK == 0) str_allocate();
      }

   virtual ~UFCGIPlugIn();

   // define method VIRTUAL of class UServerPlugIn

   // Server-wide hooks

   virtual int handlerConfig(UFileConfig& cfg);
   virtual int handlerInit();

   // Connection-wide hooks

   virtual int handlerRequest();
   virtual int handlerReset();

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   UString fcgi_uri_mask;
   UClient_Base* connection;
   bool fcgi_keep_conn;

private:
   UFCGIPlugIn(const UFCGIPlugIn&) : UServerPlugIn() {}
   UFCGIPlugIn& operator=(const UFCGIPlugIn&)        { return *this; }
};

#endif
