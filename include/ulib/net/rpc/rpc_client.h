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

   URPCClient(UFileConfig* cfg) : UClient_Base(cfg), UClient<Socket>(cfg)
      {
      U_TRACE_REGISTER_OBJECT(0, URPCClient, "%p", cfg)

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

      this->UClient_Base::request = URPCMethod::encoder->encodeMethodCall(method, UString::getStringNull());

      bool result = (this->UClient_Base::sendRequest() &&
                     this->UClient_Base::readRPCResponse() &&
                     this->UClient_Base::buffer == *URPCMethod::str_done); 

      U_RETURN(result);
      }

#ifdef DEBUG
   const char* dump(bool reset) const { return UClient<Socket>::dump(reset); }
#endif

private:
   URPCClient(const URPCClient&)            {}
   URPCClient& operator=(const URPCClient&) { return *this; }
};

#endif
