// test_unixsocket_server.cpp

#include <ulib/net/unixsocket.h>
#include <ulib/net/server/server.h>

class UUnixClientImage : public UClientImage<UUnixSocket> {
public:

   UUnixClientImage() : UClientImage<UUnixSocket>()
      {
      U_TRACE_REGISTER_OBJECT(5, UUnixClientImage, "", 0)
      }

   ~UUnixClientImage()
      {
      U_TRACE_UNREGISTER_OBJECT(5, UUnixClientImage)
      }

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const { return UClientImage<UUnixSocket>::dump(reset); }
#endif

protected:

   // define method VIRTUAL of class UEventFd

   virtual int handlerRead()
      {
      U_TRACE(5, "UUnixClientImage::handlerRead()")

      int result = genericRead();

      if (result == U_PLUGIN_HANDLER_AGAIN) U_RETURN(U_NOTIFIER_OK); // NONBLOCKING...
      if (result == U_PLUGIN_HANDLER_ERROR) U_RETURN(U_NOTIFIER_DELETE);

      UClientImage_Base::initAfterGenericRead();

      if (UServer_Base::isLog()) UClientImage_Base::logRequest();

      U_INTERNAL_ASSERT_EQUALS(result, U_PLUGIN_HANDLER_GO_ON)

      *wbuffer = *rbuffer;

      result = handlerWrite();

      U_RETURN(result);
      }
};

U_MACROSERVER(UUnixServer, UUnixClientImage, UUnixSocket);

int
U_EXPORT main (int argc, char* argv[])
{
   U_ULIB_INIT(argv);

   U_TRACE(5,"main(%d)",argc)

   UUnixServer s(0);

   UUnixSocket::setPath(argv[1]);

   s.go();
}
