// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    mod_rpc.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef U_MOD_RPC_H
#define U_MOD_RPC_H 1

#include <ulib/net/server/server_plugin.h>

class URPCParser;

class U_EXPORT URpcPlugIn : public UServerPlugIn {
public:

   // COSTRUTTORI

   URpcPlugIn()
      {
      U_TRACE_REGISTER_OBJECT(0, URpcPlugIn, "")

      rpc_parser = 0;
      is_rpc_msg = false;
      }

   virtual ~URpcPlugIn();

   // define method VIRTUAL of class UServerPlugIn

   // Server-wide hooks

   virtual int handlerConfig(UFileConfig& cfg);
   virtual int handlerInit();

   // Connection-wide hooks

   virtual int handlerREAD();
   virtual int handlerRequest();

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   URPCParser* rpc_parser;
   bool is_rpc_msg;

private:
   URpcPlugIn(const URpcPlugIn&) : UServerPlugIn() {}
   URpcPlugIn& operator=(const URpcPlugIn&)        { return *this; }
};

#endif
