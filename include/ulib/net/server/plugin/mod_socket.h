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

class UCommand;

class U_EXPORT UWebSocketPlugIn : public UServerPlugIn {
public:

   static const UString* str_MAX_MESSAGE_SIZE;

   static void str_allocate();

   // COSTRUTTORI

            UWebSocketPlugIn();
   virtual ~UWebSocketPlugIn();

   // define method VIRTUAL of class UServerPlugIn

   // Server-wide hooks

   virtual int handlerConfig(UFileConfig& cfg);
   virtual int handlerRun();

   // Connection-wide hooks

   virtual int handlerRequest();

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   static iPFpv on_message;
   static UCommand* command;

   static RETSIGTYPE handlerForSigTERM(int signo);

private:
   UWebSocketPlugIn(const UWebSocketPlugIn&) : UServerPlugIn() {}
   UWebSocketPlugIn& operator=(const UWebSocketPlugIn&)        { return *this; }
};

#endif
