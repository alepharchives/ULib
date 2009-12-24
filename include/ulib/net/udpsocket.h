// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    udpsocket.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_UDPSOCKET_H
#define ULIB_UDPSOCKET_H 1

#include <ulib/net/socket.h>

class U_EXPORT UUDPSocket : public USocket {
public:

   // COSTRUTTORI

   UUDPSocket(bool bSocketIsIPv6 = false) : USocket(bSocketIsIPv6)
      {
      U_TRACE_REGISTER_OBJECT(0, UUDPSocket, "%b", bSocketIsIPv6)
      }

   virtual ~UUDPSocket()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UUDPSocket)
      }

   // VIRTUAL METHOD

   /**
   This method is called to connect the socket to a server UDP socket that is specified
   by the provided IP Address and port number. We call the connect() method to perform the connection.
   */

   virtual bool connectServer(const UString& server, int iServPort) { return (USocket::socket(SOCK_DGRAM) && USocket::connectServer(server, iServPort)); }

   /**
   We try to bind the USocket to the specified port number and any local IP Address using the bind() method.
   */

   virtual bool setServer(                           int port, int iBackLog) { return (USocket::socket(SOCK_DGRAM) && USocket::setServer(            port, 0)); }
   virtual bool setServer(const UString& cLocalAddr, int port, int iBackLog) { return (USocket::socket(SOCK_DGRAM) && USocket::setServer(cLocalAddr, port, 0)); }

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const { return USocket::dump(reset); }
#endif

private:
   UUDPSocket(const UUDPSocket&) : USocket(false) {}
   UUDPSocket& operator=(const UUDPSocket&)       { return *this; }
};

#endif
