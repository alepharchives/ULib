// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    mod_tsa.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef U_MOD_TSA_H
#define U_MOD_TSA_H 1

#include <ulib/net/server/server_plugin.h>

class UCommand;

class U_EXPORT UTsaPlugIn : public UServerPlugIn {
public:

   // COSTRUTTORI

   UTsaPlugIn()
      {
      U_TRACE_REGISTER_OBJECT(0, UTsaPlugIn, "")

      command = 0;
      }

   virtual ~UTsaPlugIn();

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

private:
   UTsaPlugIn(const UTsaPlugIn&) : UServerPlugIn() {}
   UTsaPlugIn& operator=(const UTsaPlugIn&)        { return *this; }
};

#endif
