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

            UProxyPlugIn();
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
   static UVector<UString>* vmsg_error;
   static UHttpClient<UTCPSocket>* client_http;
   static UVector<UModProxyService*>* vservice;

private:
   UProxyPlugIn(const UProxyPlugIn&) : UServerPlugIn() {}
   UProxyPlugIn& operator=(const UProxyPlugIn&)        { return *this; }
};

#endif
