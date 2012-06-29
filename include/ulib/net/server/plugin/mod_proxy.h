// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    mod_proxy.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef U_MOD_PROXY_H
#define U_MOD_PROXY_H 1

#include <ulib/net/client/http.h>
#include <ulib/net/server/server_plugin.h>

class UModProxyService;

class U_EXPORT UProxyPlugIn : public UServerPlugIn {
public:

   // COSTRUTTORI

   UProxyPlugIn() : client_http((UFileConfig*)0)
      {
      U_TRACE_REGISTER_OBJECT(0, UProxyPlugIn, "")
      }

   virtual ~UProxyPlugIn();

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
   UVector<UString> vmsg_error;
   UHttpClient<UTCPSocket> client_http;
   UVector<UModProxyService*>* vservice;

private:
   UProxyPlugIn(const UProxyPlugIn&) : UServerPlugIn(), client_http((UFileConfig*)0) {}
   UProxyPlugIn& operator=(const UProxyPlugIn&)                                      { return *this; }
};

#endif
