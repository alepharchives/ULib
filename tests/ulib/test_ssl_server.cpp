// test_ssl_server.cpp

#include <ulib/ssl/certificate.h>
#include <ulib/net/server/server.h>
#include <ulib/ssl/net/sslsocket.h>

class USSLClientImage : public UClientImage<USSLSocket> {
public:

   USSLClientImage() : UClientImage<USSLSocket>()
      {
      U_TRACE_REGISTER_OBJECT(5, USSLClientImage, "", 0)
      }

   ~USSLClientImage()
      {
      U_TRACE_UNREGISTER_OBJECT(5, USSLClientImage)
      }

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const { return UClientImage<USSLSocket>::dump(reset); }
#endif

protected:

   // define method VIRTUAL of class UEventFd

   virtual int handlerRead()
      {
      U_TRACE(5, "USSLClientImage::handlerRead()")

      int result = genericRead();

      if (result == U_PLUGIN_HANDLER_AGAIN) U_RETURN(U_NOTIFIER_OK); // NONBLOCKING...
      if (result == U_PLUGIN_HANDLER_ERROR) U_RETURN(U_NOTIFIER_DELETE);

      UClientImage_Base::initAfterGenericRead();

      if (UServer_Base::isLog()) UClientImage_Base::logRequest();

      U_INTERNAL_ASSERT_EQUALS(result, U_PLUGIN_HANDLER_GO_ON)

      static bool init;

      if (init == false)
         {
         init = true;

         X509* x509 = getSocket()->getPeerCertificate();

         if (x509 == 0 &&
             getSocket()->askForClientCertificate())
            {
            x509 = getSocket()->getPeerCertificate();

            U_INTERNAL_ASSERT_DIFFERS(x509, 0)
            }

         if (x509) cerr << UCertificate(x509).print();
         }

      *wbuffer = *rbuffer;

      result = handlerResponse();

      U_RETURN(result);
      }
};

U_MACROSERVER(USSLServer, USSLClientImage, USSLSocket);

static const char* getArg(const char* param) { return (param && *param ? param : 0); }

int
U_EXPORT main (int argc, char* argv[])
{
   U_ULIB_INIT(argv);

   U_TRACE(5,"main(%d)",argc)

   USSLServer s(0);

   // Load our certificate

   s.getSocket()->setContext(0, getArg(argv[1]), getArg(argv[2]), getArg(argv[3]), getArg(argv[4]), getArg(argv[5]), atoi(argv[6]));

   s.run();
}
