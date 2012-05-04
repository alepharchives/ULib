// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    mod_socket.h - web socket
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef U_MOD_SOCKET_H
#define U_MOD_SOCKET_H 1

#include <ulib/net/server/server_plugin.h>

/*
The plugin interface is an integral part of UServer which provides a flexible way to add specific functionality to UServer.
Plugins allow you to enhance the functionality of UServer without changing the core of the server. They can be loaded at
startup time and can change virtually some aspect of the behaviour of the server.

UServer has 6 hooks which are used in different states of the execution of the request:
--------------------------------------------------------------------------------------------
* Server-wide hooks:
````````````````````
1) handlerConfig: called when the server finished to process its configuration
2) handlerInit:   called when the server finished its init, and before start to run
3) handlerFork:   called when the server finished its forks, and before start to run

* Connection-wide hooks:
````````````````````````
4) handlerREAD:
5) handlerRequest:
6) handlerReset:
  called in `UClientImage_Base::handlerRead()`
--------------------------------------------------------------------------------------------

RETURNS:
  U_PLUGIN_HANDLER_GO_ON    if ok
  U_PLUGIN_HANDLER_FINISHED if the final output is prepared
  U_PLUGIN_HANDLER_AGAIN    if the request is empty (NONBLOCKING)

  U_PLUGIN_HANDLER_ERROR    on error
*/

class UCommand;

class U_EXPORT UWebSocketPlugIn : public UServerPlugIn {
public:

   static const UString* str_USE_SIZE_PREAMBLE;

   static void str_allocate();

   // COSTRUTTORI

   UWebSocketPlugIn()
      {
      U_TRACE_REGISTER_OBJECT(0, UWebSocketPlugIn, "")

      command = 0;

      if (str_USE_SIZE_PREAMBLE == 0) str_allocate();
      }

   virtual ~UWebSocketPlugIn();

   // define method VIRTUAL of class UServerPlugIn

   // Server-wide hooks

   virtual int handlerConfig(UFileConfig& cfg);
   virtual int handlerInit();

   // Connection-wide hooks

   virtual int handlerRequest();

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   UCommand* command;

   static bool bUseSizePreamble;

   static RETSIGTYPE handlerForSigTERM(int signo);

   static bool handleDataFraming(USocket* csocket);
   static void getPart(const char* key, unsigned char* part);

private:
   UWebSocketPlugIn(const UWebSocketPlugIn&) : UServerPlugIn() {}
   UWebSocketPlugIn& operator=(const UWebSocketPlugIn&)        { return *this; }
};

#endif
