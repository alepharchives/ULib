// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    tcpsocket.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/net/tcpsocket.h>

// VIRTUAL METHOD

#ifdef closesocket
#undef closesocket
#endif

void UTCPSocket::closesocket()
{
   U_TRACE(1, "UTCPSocket::closesocket()")

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT(USocket::isOpen())

   U_INTERNAL_DUMP("isBroken() = %b", USocket::isBroken())

   if (USocket::isBroken())
      {
      /*
      static struct linger l = { 1, 0 };

      (void) setSockOpt(SOL_SOCKET, SO_LINGER, (const void*)&l, sizeof(struct linger)); // send RST - ECONNRESET
      */

      /* The shutdown() tells the receiver the server is done sending data. No
       * more data is going to be send. More importantly, it doesn't close the
       * socket. At the socket layer, this sends a TCP/IP FIN packet to the receiver
       */

      if (USocket::shutdown())
         {
         /* At this point, the socket layer has to wait until the receiver has
          * acknowledged the FIN packet by receiving a ACK packet. This is done by
          * using the recv() command in a loop until 0 or less value is returned.
          * Once recv() returns 0 (or less), 1/2 of the socket is closed
          */

         char buf[8*1024];
         uint32_t count = 0;

         do { if (count++ > 5) break; errno = 0; } while (USocket::recv(iSockDesc, buf, sizeof(buf)) > 0 || errno == EAGAIN);
         }
      }

   // Then you can close the second half of the socket by calling closesocket()

   USocket::closesocket();
}
