// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    mod_tcc.h - plugin userver for TCC (Tiny C Compiler)
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef U_MOD_TCC_H
#define U_MOD_TCC_H 1

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

class U_EXPORT UTCCPlugIn : public UServerPlugIn {
public:

   // COSTRUTTORI

   UTCCPlugIn()
      {
      U_TRACE_REGISTER_OBJECT(0, UTCCPlugIn, "")
      }

   virtual ~UTCCPlugIn()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UTCCPlugIn)
      }

   // define method VIRTUAL of class UServerPlugIn

   // Server-wide hooks

   virtual int handlerInit();

   // Connection-wide hooks

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:

private:
   UTCCPlugIn(const UTCCPlugIn&) : UServerPlugIn() {}
   UTCCPlugIn& operator=(const UTCCPlugIn&)        { return *this; }
};

#endif
