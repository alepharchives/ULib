// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    mod_usp.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef U_MOD_USP_H
#define U_MOD_USP_H 1

#include <ulib/container/hash_map.h>
#include <ulib/net/server/server_plugin.h>

/*
The plugin interface is an integral part of UServer which provides a flexible way to add specific functionality to UServer.
Plugins allow you to enhance the functionality of UServer without changing the core of the server. They can be loaded at
startup time and can change virtually some aspect of the behaviour of the server.

UServer has 5 hooks which are used in different states of the execution of the request:
--------------------------------------------------------------------------------------------
* Server-wide hooks:
````````````````````
1) loadConfigParam: called when the server finished to process its configuration
2) handlerInit:     called when the server finished its init, and before start to run

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

class U_EXPORT UUspPlugIn : public UServerPlugIn {
public:

   // COSTRUTTORI

   UUspPlugIn()
      {
      U_TRACE_REGISTER_OBJECT(0, UUspPlugIn, "", 0)

      pages.allocate();
      }

   virtual ~UUspPlugIn();

   // define method VIRTUAL of class UServerPlugIn

   // Server-wide hooks

   virtual int handlerConfig(UFileConfig& cfg)
      {
      U_TRACE(0, "UUspPlugIn::handlerConfig(%p)", &cfg)

      U_RETURN(U_PLUGIN_HANDLER_GO_ON);
      }

   virtual int handlerInit();

   // Connection-wide hooks

   virtual int handlerRead()
      {
      U_TRACE(0, "UUspPlugIn::handlerRead()")

      U_RETURN(U_PLUGIN_HANDLER_GO_ON);
      }

   virtual int handlerRequest();

   virtual int handlerReset()
      {
      U_TRACE(0, "UUspPlugIn::handlerReset()")

      U_INTERNAL_ASSERT_POINTER(runDynamicPage)

      runDynamicPage((void*)-1); // call reset for module...

      U_RETURN(U_PLUGIN_HANDLER_GO_ON);
      }

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   UString last_key;
   UDynamic* last_page;
   UHashMap<UDynamic*> pages;

   static void* argument;
   static vPFpv runDynamicPage;

   static void callRunDynamicPage(UStringRep* key, void* value);

private:
   UUspPlugIn(const UUspPlugIn&) : UServerPlugIn() {}
   UUspPlugIn& operator=(const UUspPlugIn&)        { return *this; }
};

#endif
