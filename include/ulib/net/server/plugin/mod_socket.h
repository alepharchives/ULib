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
