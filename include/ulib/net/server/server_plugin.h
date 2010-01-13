// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    server_plugin.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef U_SERVER_PLUGIN_H
#define U_SERVER_PLUGIN_H 1

#include <ulib/dynamic/plugin.h>

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

enum PluginHandlerReturn { U_PLUGIN_HANDLER_ERROR    = -1,
                           U_PLUGIN_HANDLER_GO_ON    =  0,
                           U_PLUGIN_HANDLER_FINISHED =  1,
                           U_PLUGIN_HANDLER_AGAIN    =  2 };

class UFileConfig;

class UServerPlugIn {
public:

   // COSTRUTTORE

            UServerPlugIn() {}
   virtual ~UServerPlugIn() {}

   // Server-wide hooks

   virtual int handlerConfig(UFileConfig& cfg) = 0;
   virtual int handlerInit()                   = 0;

   // Connection-wide hooks

   virtual int handlerRead()    = 0;
   virtual int handlerRequest() = 0;
   virtual int handlerReset()   = 0;

private:
   UServerPlugIn(const UServerPlugIn&)            {}
   UServerPlugIn& operator=(const UServerPlugIn&) { return *this; }
};

#endif
