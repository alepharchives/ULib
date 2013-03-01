// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    socket.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_SOCKET_H
#define ULIB_SOCKET_H 1

#include <ulib/net/ipaddress.h>

#ifdef HAVE_SYS_SENDFILE_H
#  ifndef HAVE_SENDFILE64
#     undef __USE_FILE_OFFSET64
#  endif
#  include <sys/sendfile.h>
#  ifndef HAVE_SENDFILE64
#     define __USE_FILE_OFFSET64
#  endif
#endif

#ifdef __MINGW32__
#  include <ws2tcpip.h>
#  define CAST(a) (char*)a
#else
#  define CAST(a) a
#  include <netinet/tcp.h>
#endif

#include <errno.h>

#if !defined(ENABLE_IPV6) && !defined(AF_INET6)
#  define AF_INET6 AF_INET
#endif

#ifndef SOL_TCP
#define SOL_TCP IPPROTO_TCP
#endif

/* Atomically mark descriptor(s) as non-blocking */
#ifndef SOCK_NONBLOCK
#define SOCK_NONBLOCK   04000
#endif
/* Atomically set close-on-exec flag for the new descriptor(s) */
#ifndef SOCK_CLOEXEC
#define SOCK_CLOEXEC    02000000
#endif

/**
   @class USocket

   @brief basic IP socket functionality

   This class is used to provide basic IP socket functionality within a C++ class environment.
   The socket descriptor is stored as a member variable within the socket class.
   The member methods are simple wrappers to the standard socket library function calls except
   they use class UIPAddress instances and port numbers rather than sockaddr structures
*/

class UHTTP;
class UFile;
class UNotifier;
class USocketExt;
class USSHSocket;
class USSLSocket;
class UTCPSocket;
class UUnixSocket;
class USmtpClient;
class UImapClient;
class UPop3Client;
class UClient_Base;
class UServer_Base;
class SocketAddress;
class UClientImage_Base;

#define U_socket_IPv6(obj)     (obj)->USocket::flag[0]
#define U_socket_LocalSet(obj) (obj)->USocket::flag[1]
#define U_socket_unused1(obj)  (obj)->USocket::flag[2]
#define U_socket_unused2(obj)  (obj)->USocket::flag[3]

class U_EXPORT USocket {
public:

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   static const UString* str_host;
   static const UString* str_range;
   static const UString* str_close;
   static const UString* str_cookie;
   static const UString* str_setcookie;
   static const UString* str_starttls;
   static const UString* str_location;
   static const UString* str_refresh;
   static const UString* str_connection;
   static const UString* str_user_agent;
   static const UString* str_authorization;
   static const UString* str_content_type;
   static const UString* str_content_length;
   static const UString* str_content_disposition;
   static const UString* str_accept;
   static const UString* str_accept_language;
   static const UString* str_accept_encoding;
   static const UString* str_if_range;
   static const UString* str_if_none_match;
   static const UString* str_if_modified_since;
   static const UString* str_if_unmodified_since;
   static const UString* str_referer;
   static const UString* str_X_Real_IP;
   static const UString* str_X_Forwarded_For;
   static const UString* str_Transfer_Encoding;
   static const UString* str_X_Progress_ID;
   static const UString* str_expect_100_continue;
   static const UString* str_chunked;

   static void str_allocate();

   // COSTRUTTORI

   static int accept4_flags; // If flags is 0, then accept4() is the same as accept()

   enum State {
      CLOSE       = 0x000,
      TIMEOUT     = 0x001,
      BROKEN      = 0x002,
      EPOLLERROR  = 0x004,
      CONNECT     = 0x008,
      LOGIN       = 0x010
   };

            USocket(bool bSocketIsIPv6 = false);
   virtual ~USocket();

#ifdef __MINGW32__
   int getFd() const { return fh; }
#else
   int getFd() const { return iSockDesc; }
#endif

   bool isOpen() const      { return (iSockDesc > 0); }
   bool isLogin() const     { return (iState == LOGIN); }
   bool isClosed() const    { return (iSockDesc <= 0); }
   bool isBroken() const    { return ((iState & BROKEN)     != 0); }
   bool isTimeout() const   { return ((iState & TIMEOUT)    != 0); }
   bool isEpollErr() const  { return ((iState & EPOLLERROR) != 0); }
   bool isSysError() const  { return (iState  < CLOSE); }
   bool isConnected() const { return (iState >= CONNECT); }

   /**
   This method is called after read block of data of remote connection
   */

   void checkErrno();
   bool checkTime(long time_limit, long& timeout);

   /**
   This method is called after send block of data to remote connection
   */

   bool checkIO(int iBytesTransferred)
      {
      U_TRACE(0, "USocket::checkIO(%d)", iBytesTransferred)

      if (iBytesTransferred <= 0)
         {
         if (iBytesTransferred < 0) checkErrno();

         U_RETURN(false);
         }

      U_RETURN(true);
      }

   /**
   The socket() function is called to create the socket of the specified type.
   The parameters indicate whether the socket will use IPv6 or IPv4 and the type of socket
   (the default being SOCK_STREAM or TCP). The returned descriptor is stored in iSockDesc
   */

   static int socket(int domain, int type, int protocol)
      {
      U_TRACE(1, "USocket::socket(%d,%d,%d)", domain, type, protocol)

      int fd = U_SYSCALL(socket, "%d,%d,%d", domain, type, protocol);

      U_RETURN(fd);
      }

   bool socket(int iSocketType, int protocol = 0);

   void close()
      {
      U_TRACE(0, "USocket::close()")

      closesocket();

      iState = CLOSE;
      }

   /**
   The getsockopt() function is called with the provided parameters to obtain the desired value
   */

   bool getSockOpt(int iCodeLevel, int iOptionName, void* pOptionData, uint32_t& iDataLength)
      {
      U_TRACE(1, "USocket::getSockOpt(%d,%d,%p,%p)", iCodeLevel, iOptionName, pOptionData, iDataLength)

      U_CHECK_MEMORY

      U_INTERNAL_ASSERT(isOpen())

      bool result = (U_SYSCALL(getsockopt, "%d,%d,%d,%p,%p", getFd(), iCodeLevel, iOptionName, CAST(pOptionData), (socklen_t*)&iDataLength) == 0);

      U_RETURN(result);
      }

   /**
   The setsockopt() function is called with the provided parameters to obtain the desired value
   */

   bool setSockOpt(int iCodeLevel, int iOptionName, const void* pOptionData, uint32_t iDataLength)
      {
      U_TRACE(1, "USocket::setSockOpt(%d,%d,%p,%u)", iCodeLevel, iOptionName, pOptionData, iDataLength)

      U_CHECK_MEMORY

      U_INTERNAL_ASSERT(isOpen())

      bool result = (U_SYSCALL(setsockopt, "%d,%d,%d,%p,%u", getFd(), iCodeLevel, iOptionName, CAST(pOptionData), iDataLength) == 0);

      U_RETURN(result);
      }

   /**
   Actual state is blocking...?
   */

   bool isBlocking()
      {
      U_TRACE(0, "USocket::isBlocking()")

      U_CHECK_MEMORY

      U_INTERNAL_ASSERT(isOpen())

      U_INTERNAL_DUMP("O_NONBLOCK = %B, flags = %B", O_NONBLOCK, flags)

      bool blocking = ((flags & O_NONBLOCK) != O_NONBLOCK);

      U_RETURN(blocking);
      }

   /**
   Connect the socket to the specified server IP Address and port number
   */

   bool connectServer(const UIPAddress& cAddr, int iServPort);

   /**
   The method is called with a local IP address and port number to bind the socket to.
   A default port number of zero is a wildcard and lets the OS choose the port number
   */

   bool bind(                        int iLocalPort = 0);
   bool bind(UIPAddress& cLocalAddr, int iLocalPort = 0);

   /**
   The iBackLog parameter indicates the number of unconnected sockets that can be pending in the socket queue
   */

   void listen(int iBackLog = 5)
      {
      U_TRACE(1, "USocket::listen(%d)", iBackLog)

      U_CHECK_MEMORY

      U_INTERNAL_ASSERT(isOpen())

      (void) U_SYSCALL(listen, "%d,%d", getFd(), iBackLog);
      }

   /**
   Get details of the IP address and port number bound to the local socket
   */

   UIPAddress& localIPAddress()  __pure;
   int         localPortNumber() __pure;

   const char* getLocalInfo()
      {
      U_TRACE(0, "USocket::getLocalInfo()")

      U_CHECK_MEMORY

      U_INTERNAL_ASSERT(isLocalSet())

      const char* address = cLocalAddress.getAddressString();

      U_RETURN(address);
      }

   /**
   This method is called when the local socket address is valid.
   It sets the LocalSet flag, obtains details of the local address and stores them in the internal member variables
   */

   void setLocal();
   void setLocal(const UIPAddress& addr)
      {
      U_TRACE(0, "USocket::setLocal(%p)", &addr)

      cLocalAddress = addr;

      U_socket_LocalSet(this) = true;
      }

   bool isLocalSet() const { return U_socket_LocalSet(this); }

   /**
   Get details of the IP address and port number bound to the remote socket
   */

   int remotePortNumber()
      {
      U_TRACE(0, "USocket::remotePortNumber()")

      U_CHECK_MEMORY

      U_RETURN(iRemotePort);
      }

   UIPAddress& remoteIPAddress()
      {
      U_TRACE(0, "USocket::remoteIPAddress()")

      U_CHECK_MEMORY

      return cRemoteAddress;
      }

   /**
   This method manage the buffer of the socket connection
   */

   uint32_t getBufferRCV()
      {
      U_TRACE(1, "USocket::getBufferRCV()")

      uint32_t size = U_NOT_FOUND, tmp = sizeof(uint32_t);

      (void) getSockOpt(SOL_SOCKET, SO_RCVBUF, (void*)&size, tmp);

      U_RETURN(size);
      }

   uint32_t getBufferSND()
      {
      U_TRACE(1, "USocket::getBufferSND()")

      uint32_t size = U_NOT_FOUND, tmp = sizeof(uint32_t);

      (void) getSockOpt(SOL_SOCKET, SO_SNDBUF, (void*)&size, tmp);

      U_RETURN(size);
      }

   bool setBufferRCV(uint32_t size)
      {
      U_TRACE(1, "USocket::setBufferRCV(%u)", size)

      bool result = setSockOpt(SOL_SOCKET, SO_RCVBUF, (const void*)&size, sizeof(uint32_t));

      U_RETURN(result);
      }

   bool setBufferSND(uint32_t size)
      {
      U_TRACE(1, "USocket::setBufferSND(%u)", size)

      bool result = setSockOpt(SOL_SOCKET, SO_SNDBUF, (const void*)&size, sizeof(uint32_t));

      U_RETURN(result);
      }

   /* The shutdown() tells the receiver the server is done sending data. No
    * more data is going to be send. More importantly, it doesn't close the
    * socket. At the socket layer, this sends a TCP/IP FIN packet to the receiver
    */

   bool shutdown(int how = SHUT_WR)
      {
      U_TRACE(1, "USocket::shutdown(%d)", how)

      U_CHECK_MEMORY

      U_INTERNAL_ASSERT(isOpen())

      bool result = (U_SYSCALL(shutdown, "%d,%d", getFd(), how) == 0);

      U_RETURN(result);
      }

   /**
    * Stick a TCP cork in the socket. It's not clear that this will help performance, but it might.
    *
    * TCP_CORK: If set, don't send out partial frames. All queued partial frames are sent when the option is cleared again. This is useful
    *           for  prepending headers before calling sendfile(), or for throughput optimization. As currently implemented, there is a 200
    *           millisecond ceiling on the time for which output is corked by TCP_CORK. If this ceiling is reached, then queued data is
    *           automatically transmitted.
    *
    * This is a no-op if we don't think this platform has corks.
    ***/

   void setTcpCork(uint32_t corked)
      {
      U_TRACE(0, "USocket::setTcpCork(%u)", corked)

#  ifdef TCP_CORK
      (void) setSockOpt(SOL_TCP, TCP_CORK, (const void*)&corked, sizeof(uint32_t));
#  endif
      }

   /**
    * Ask for the server not to be awakened until some data has arrived on the socket. Takes an integer value (seconds), this can bound the
    * maximum number of attempts TCP will make to complete the connection. This works for RPC protocol because the client sends a request immediately
    * after connection without waiting for anything from the server.
    ***/

   void setTcpDeferAccept(uint32_t value)
      {
      U_TRACE(0, "USocket::setTcpDeferAccept(%u)", value)

#  ifdef TCP_DEFER_ACCEPT
      (void) setSockOpt(SOL_TCP, TCP_DEFER_ACCEPT, (const void*)&value, sizeof(uint32_t));
#  endif
      }

   void setTcpQuickAck(uint32_t value)
      {
      U_TRACE(0, "USocket::setTcpQuickAck(%u)", value)

#  ifdef TCP_QUICKACK
      (void) setSockOpt(SOL_TCP, TCP_QUICKACK, (const void*)&value, sizeof(uint32_t));
#  endif
      }

   void setTcpNoDelay(uint32_t value)
      {
      U_TRACE(0, "USocket::setTcpNoDelay(%u)", value)

#  ifdef TCP_NODELAY
      (void) setSockOpt(SOL_TCP, TCP_NODELAY, (const void*)&value, sizeof(uint32_t));
#  endif
      }

   void setTcpFastOpen(uint32_t value)
      {
      U_TRACE(0, "USocket::setTcpFastOpen(%u)", value)

#  ifdef TCP_FASTOPEN
      (void) setSockOpt(SOL_TCP, TCP_FASTOPEN, (const void*)&value, sizeof(uint32_t));
#  endif
      }

   void setTcpCongestion(const char* value)
      {
      U_TRACE(0, "USocket::setTcpCongestion(%S)", value)

#  ifdef TCP_CONGESTION
      (void) setSockOpt(IPPROTO_TCP, TCP_CONGESTION, (const void*)&value, u__strlen(value) + 1);
#  endif
      }

   /**
   Enables/disables the @c SO_TIMEOUT pseudo option

   @c SO_TIMEOUT is not one of the options defined for Berkeley sockets, but was actually introduced
   as part of the Java API. For client sockets it has the same meaning as the @c SO_RCVTIMEO option,
   which specifies the maximum number of milliseconds that a blocking @c read() call will wait for
   data to arrive on the socket. Timeouts only have effect for system calls that perform socket I/O
   (e.g., read(2), recvmsg(2), send(2), sendmsg(2));

   @param timeoutMS the specified timeout value, in milliseconds. A zero value indicates no timeout, i.e. an infinite wait
   */

   bool setTimeoutRCV(uint32_t timeoutMS = U_TIMEOUT_MS);
   bool setTimeoutSND(uint32_t timeoutMS = U_TIMEOUT_MS);

   /**
   The recvfrom() function is called with the proper parameters, params is placed for obtaining
   the source address information. The number of bytes read is returned
   */

   int recvFrom(void* pBuffer, uint32_t iBufLength, uint32_t uiFlags, UIPAddress& cSourceIP, int& iSourcePortNumber);

   /**
   The socket transmits the data to the remote socket.
   */

   int sendTo(void* pPayload, uint32_t iPayloadLength, uint32_t uiFlags, UIPAddress& cDestinationIP, int iDestinationPortNumber);

   /**
   This method is called to receive a block of data on the connected socket.
   The parameters signify the payload receiving buffer and its size.
   If the socket is not connected, then we failed on assertion, otherwise we call
   the recv() system call to receive the data, returning the number of bytes actually readden
   */

   static int recv(int fd, void* buf, uint32_t len, int recv_flags);

   /**
   This method is called to read a 16-bit binary value from the remote connection.
   We loop - calling recv() - until the required number of bytes are read, if recv()
   returns a smaller number of bytes due to the remaining values not yet arriving, we go back into a loop.
   Once the value is read into uiNetOrder, we convert it to host byte order and return the read value
   */

   int recvBinary16Bits();

   /**
   This method is called to read a 32-bit binary value from the remote connection.
   We loop - calling recv() - until the required number of bytes are read, if recv()
   returns a smaller number of bytes due to the remaining values not yet arriving, we go back into a loop.
   Once the value is read into uiNetOrder, we convert it to host byte order and return the read value
   */

   uint32_t recvBinary32Bits();

   /**
   This method is called to send a 16-bit binary value to the remote connection.
   We convert the parameter to network byte order and call the send() method to send it.
   If two bytes are not sent (the returned value is not two), return false
   */

   bool sendBinary16Bits(uint16_t iData);

   /**
   This method is called to send a 32-bit binary value to the remote connection.
   We convert the parameter to network byte order and call the send() method to send it.
   If four bytes are not sent (the returned value is not four), return false
   */

   bool sendBinary32Bits(uint32_t lData);

   // -----------------------------------------------------------------------------------------------------------
   // VIRTUAL METHOD
   // -----------------------------------------------------------------------------------------------------------

   virtual bool isSSL() const
      {
      U_TRACE(0, "USocket::isSSL()")

      U_RETURN(false);
      }

   virtual bool isIPC() const
      {
      U_TRACE(0, "USocket::isIPC()")

      U_RETURN(false);
      }

#ifdef closesocket
#undef closesocket
#endif

   virtual void closesocket() { _closesocket(); }

   virtual const char* getMsgError(char* buffer, uint32_t buffer_size);

   /**
   This method is called to connect the socket to a server TCP socket that is specified
   by the provided IP Address and port number. We call the connect() method to perform the connection
   */

   virtual bool connectServer(const UString& server, int iServPort);

   /**
   The default local port number is automatically allocated, the default back logged queue length is 5.
   We then try to bind the USocket to the specified port number and any local IP Address using the bind() method.
   Following this, we call the listen() method to cause the socket to begin listening for new connections
   */

   virtual bool setServer(                           int port, int iBackLog);
   virtual bool setServer(const UString& cLocalAddr, int port, int iBackLog);

   /**
   This method is called to accept a new pending connection on the server socket.
   The USocket pointed to by the provided parameter is modified to refer to the
   newly connected socket. The remote IP Address and port number are also set.
   */

   virtual bool acceptClient(USocket* pcConnection);

   /**
   This method is called to receive a block of data on the connected socket.
   The parameters signify the payload receiving buffer and its size.
   If the socket is not connected, then we failed on assertion, otherwise we call
   the recv() method to receive the data, returning the number of bytes actually readden
   */

           int recv(void* pBuffer, uint32_t iBufLength, int timeoutMS);
   virtual int recv(void* pBuffer, uint32_t iBufLength);

   /**
   This method is called to send a block of data to the remote connection.
   The parameters signify the Data Payload and its size.
   If the socket is not connected, then we failed on assertion, otherwise we call
   the send() method to send the data, returning the number of bytes actually sent
   */

   virtual int send(const char* pPayload, uint32_t iPayloadLength);
           int send(const void* pPayload, uint32_t iPayloadLength, int timeoutMS);

   // write data into multiple buffers

   virtual int writev(const struct iovec* iov, int iovcnt);
           int writev(const struct iovec* iov, int iovcnt, int timeoutMS);
   // -----------------------------------------------------------------------------------------------------------

   /*
   sendfile() copies data between one file descriptor and another. Either or both of these file descriptors may refer to a socket.
   OUT_FD should be a descriptor opened for writing. POFFSET is a pointer to a variable holding the input file pointer position from
   which sendfile() will start reading data. When sendfile() returns, this variable will be set to the offset of the byte following
   the last byte that was read. COUNT is the number of bytes to copy between file descriptors. Because this copying is done within
   the kernel, sendfile() does not need to spend time transferring data to and from user space.
   */

   bool sendfile(int in_fd, off_t* poffset, uint32_t count, int timeoutMS);

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   UIPAddress cLocalAddress, cRemoteAddress;
   int iSockDesc, iState, iLocalPort, iRemotePort, flags;
   unsigned char flag[4];
#ifdef __MINGW32__
   SOCKET fh;
#endif

   bool connect();
   void setRemote();
   void _closesocket();
   bool bind(SocketAddress& cLocal);
   int  _writev(const struct iovec* iov, int iovcnt);
   bool setServer(SocketAddress& cLocal, int iBackLog);

private:
   USocket(const USocket&)            {}
   USocket& operator=(const USocket&) { return *this; }

                      friend class UHTTP;
                      friend class UFile;
                      friend class UNotifier;
                      friend class USocketExt;
                      friend class USSHSocket;
                      friend class UTCPSocket;
                      friend class USSLSocket;
                      friend class UUnixSocket;
                      friend class UFtpClient;
                      friend class USmtpClient;
                      friend class UImapClient;
                      friend class UPop3Client;
                      friend class UClient_Base;
                      friend class UServer_Base;
                      friend class UClientImage_Base;
                      friend class UStreamPlugIn;
                      friend class UWebSocketPlugIn;
   template <class T> friend class UServer;
   template <class T> friend class UClientImage;
};

#endif
