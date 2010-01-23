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

      reset(); // virtual method

      int result = (USocketExt::read(socket, *rbuffer) ? U_NOTIFIER_OK
                                                       : U_NOTIFIER_DELETE);

      // check if close connection... (read() == 0)

      if (UClientImage_Base::socket->isClosed()) U_RETURN(U_NOTIFIER_DELETE);
      if (UClientImage_Base::rbuffer->empty())   U_RETURN(U_NOTIFIER_OK);

      if (UServer_Base::isLog()) UClientImage_Base::logRequest();

      if (result == U_NOTIFIER_OK)
         {
         // manage buffered read (pipelining)

         *wbuffer = rbuffer->substr(0U, (USocketExt::size_message = rbuffer->size()));

         result = handlerWrite();
         }

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
