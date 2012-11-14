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

            URpcPlugIn();
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
   static bool is_rpc_msg;
   static URPCParser* rpc_parser;

private:
   URpcPlugIn(const URpcPlugIn&) : UServerPlugIn() {}
   URpcPlugIn& operator=(const URpcPlugIn&)        { return *this; }
};

#endif
