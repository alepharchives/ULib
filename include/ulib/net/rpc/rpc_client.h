// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    rpc_client.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_RPC_CLIENT_H
#define ULIB_RPC_CLIENT_H 1

#include <ulib/net/client/client.h>
#include <ulib/net/rpc/rpc_encoder.h>

template <class Socket> class U_EXPORT URPCClient : public UClient<Socket> {
public:

   // Costruttori

   URPCClient(UFileConfig* cfg) : UClient<Socket>(cfg)
      {
      U_TRACE_REGISTER_OBJECT(0, URPCClient, "%p", cfg)

      if (cfg && UClient_Base::getServer().empty()) UClient_Base::loadConfigParam(*cfg);

      URPCMethod::encoder = U_NEW(URPCEncoder);
      }

   ~URPCClient()
      {
      U_TRACE_UNREGISTER_OBJECT(0, URPCClient)

      U_INTERNAL_ASSERT_POINTER(URPCMethod::encoder)

      delete URPCMethod::encoder;
      }

   // SERVICES

   bool processRequest(URPCMethod& method)
      {
      U_TRACE(0, "URPCClient::processRequest(%p)", &method)

      U_INTERNAL_ASSERT_POINTER(URPCMethod::encoder)

      UString request = URPCMethod::encoder->encodeMethodCall(method, UString::getStringNull());

      bool result = (this->UClient_Base::sendRequest(request,false) &&
                     this->UClient_Base::readRPCResponse()          &&
                     this->UClient_Base::buffer == *URPCMethod::str_done); 

      U_RETURN(result);
      }

#ifdef DEBUG
   const char* dump(bool _reset) const { return UClient<Socket>::dump(_reset); }
#endif

private:
   URPCClient(const URPCClient&)            {}
   URPCClient& operator=(const URPCClient&) { return *this; }
};

#endif
