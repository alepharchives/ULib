// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    mod_echo.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef U_MOD_ECHO_H
#define U_MOD_ECHO_H 1

#include <ulib/net/server/server_plugin.h>

class U_EXPORT UEchoPlugIn : public UServerPlugIn {
public:

   // COSTRUTTORE

            UEchoPlugIn() : UServerPlugIn() {}
   virtual ~UEchoPlugIn()                   {}

   // define method VIRTUAL of class UServerPlugIn

   // Server-wide hooks

   // Connection-wide hooks

   virtual int handlerRequest();

private:
   UEchoPlugIn(const UEchoPlugIn&) : UServerPlugIn() {}
   UEchoPlugIn& operator=(const UEchoPlugIn&)        { return *this; }
};

#endif
