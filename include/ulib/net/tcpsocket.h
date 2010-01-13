// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    tcpsocket.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_TCPSOCKET_H
#define ULIB_TCPSOCKET_H 1

#include <ulib/net/socket.h>

class U_EXPORT UTCPSocket : public USocket {
public:

   // COSTRUTTORI

   UTCPSocket(bool bSocketIsIPv6 = false) : USocket(bSocketIsIPv6)
      {
      U_TRACE_REGISTER_OBJECT(0, UTCPSocket, "%b", bSocketIsIPv6)
      }

   virtual ~UTCPSocket()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UTCPSocket)
      }

   // VIRTUAL METHOD

   /**
    * This method is called to connect the socket to a server TCP socket that is specified
    * by the provided IP Address and port number. We call the connect() method to perform the connection.
    */

   virtual bool connectServer(const UString& server, int iServPort)
      { return (USocket::socket(SOCK_STREAM) && USocket::connectServer(server, iServPort)); }

   /**
   * We try to bind the USocket to the specified port number and any local IP Address using the bind() method.
   */

   virtual bool setServer(int port, int iBackLog)
      { return (USocket::socket(SOCK_STREAM) && USocket::setServer(port, iBackLog)); }

   virtual bool setServer(const UString& cLocalAddr, int port, int iBackLog)
      { return (USocket::socket(SOCK_STREAM) && USocket::setServer(cLocalAddr, port, iBackLog)); }

   /**
    * This method is called to accept a new connection on the server socket.
    * Further communications on the newly connected socket are made via the newly
    * created UTCPSocket instance of which a pointer is returned when the connection is accepted.
    * We create a UTCPSocket instance and pass this to the base class accept() method to accept
    * the pending connection on this UTCPSocket instance.
   */

   virtual USocket* acceptClient(USocket* pcNewConnection)
      {
      U_TRACE(0, "UTCPSocket::acceptClient(%p)", pcNewConnection)

      (void) USocket::accept(pcNewConnection);
      
      U_RETURN_POINTER(pcNewConnection,USocket);
      }

   virtual USocket* acceptClient()
      {
      U_TRACE(0, "UTCPSocket::acceptClient()")

      USocket* pcNewConnection = U_NEW(UTCPSocket(bIPv6Socket));

      (void) USocket::accept(pcNewConnection);

      pcNewConnection->bLocalSet     = true;
      pcNewConnection->iLocalPort    = iLocalPort;
      pcNewConnection->cLocalAddress = cLocalAddress;

      U_RETURN_POINTER(pcNewConnection,USocket);
      }

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const { return USocket::dump(reset); }
#endif

private:
   UTCPSocket(const UTCPSocket&) : USocket(false) {}
   UTCPSocket& operator=(const UTCPSocket&)       { return *this; }
};

#endif
