// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    socket.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/timeval.h>
#include <ulib/net/socket.h>
#include <ulib/utility/interrupt.h>

#include <errno.h>

#ifdef __MINGW32__
#  include <ws2tcpip.h>
#endif

#include "socket_address.cpp"

UString* USocket::str_host;
UString* USocket::str_range;
UString* USocket::str_close;
UString* USocket::str_cookie;
UString* USocket::str_setcookie;
UString* USocket::str_starttls;
UString* USocket::str_location;
UString* USocket::str_connection;
UString* USocket::str_user_agent;
UString* USocket::str_authorization;
UString* USocket::str_content_type;
UString* USocket::str_content_length;
UString* USocket::str_content_disposition;
UString* USocket::str_accept_language;
UString* USocket::str_accept_encoding;
UString* USocket::str_if_range;
UString* USocket::str_if_none_match;
UString* USocket::str_if_modified_since;
UString* USocket::str_if_unmodified_since;
UString* USocket::str_referer;
UString* USocket::str_X_Forwarded_For;

void USocket::str_allocate()
{
   U_TRACE(0, "USocket::str_allocate()")

   U_INTERNAL_ASSERT_EQUALS(str_host,0)
   U_INTERNAL_ASSERT_EQUALS(str_range,0)
   U_INTERNAL_ASSERT_EQUALS(str_close,0)
   U_INTERNAL_ASSERT_EQUALS(str_cookie,0)
   U_INTERNAL_ASSERT_EQUALS(str_setcookie,0)
   U_INTERNAL_ASSERT_EQUALS(str_starttls,0)
   U_INTERNAL_ASSERT_EQUALS(str_location,0)
   U_INTERNAL_ASSERT_EQUALS(str_connection,0)
   U_INTERNAL_ASSERT_EQUALS(str_user_agent,0)
   U_INTERNAL_ASSERT_EQUALS(str_authorization,0)
   U_INTERNAL_ASSERT_EQUALS(str_content_type,0)
   U_INTERNAL_ASSERT_EQUALS(str_content_length,0)
   U_INTERNAL_ASSERT_EQUALS(str_content_disposition,0)
   U_INTERNAL_ASSERT_EQUALS(str_accept_language,0)
   U_INTERNAL_ASSERT_EQUALS(str_accept_encoding,0)
   U_INTERNAL_ASSERT_EQUALS(str_if_range,0)
   U_INTERNAL_ASSERT_EQUALS(str_if_none_match,0)
   U_INTERNAL_ASSERT_EQUALS(str_if_modified_since,0)
   U_INTERNAL_ASSERT_EQUALS(str_if_unmodified_since,0)
   U_INTERNAL_ASSERT_EQUALS(str_referer,0)
   U_INTERNAL_ASSERT_EQUALS(str_X_Forwarded_For,0)

   static ustringrep stringrep_storage[] = {
      { U_STRINGREP_FROM_CONSTANT("Host") },
      { U_STRINGREP_FROM_CONSTANT("Range") },
      { U_STRINGREP_FROM_CONSTANT("close") },
      { U_STRINGREP_FROM_CONSTANT("Cookie") },
      { U_STRINGREP_FROM_CONSTANT("Set-Cookie") },
      { U_STRINGREP_FROM_CONSTANT("STARTTLS") },
      { U_STRINGREP_FROM_CONSTANT("Location") },
      { U_STRINGREP_FROM_CONSTANT("Connection") },
      { U_STRINGREP_FROM_CONSTANT("User-Agent") },
      { U_STRINGREP_FROM_CONSTANT("Authorization") },
      { U_STRINGREP_FROM_CONSTANT("Content-Type") },
      { U_STRINGREP_FROM_CONSTANT("Content-Length") },
      { U_STRINGREP_FROM_CONSTANT("Content-Disposition") },
      { U_STRINGREP_FROM_CONSTANT("Accept-Language") },
      { U_STRINGREP_FROM_CONSTANT("Accept-Encoding") },
      { U_STRINGREP_FROM_CONSTANT("If-Range") },
      { U_STRINGREP_FROM_CONSTANT("If-None-Match") },
      { U_STRINGREP_FROM_CONSTANT("If-Modified-Since") },
      { U_STRINGREP_FROM_CONSTANT("If-Unmodified-Since") },
      { U_STRINGREP_FROM_CONSTANT("Referer") },
      { U_STRINGREP_FROM_CONSTANT("X-Forwarded-For") }
   };

   U_NEW_ULIB_OBJECT(str_host,                  U_STRING_FROM_STRINGREP_STORAGE(0));
   U_NEW_ULIB_OBJECT(str_range,                 U_STRING_FROM_STRINGREP_STORAGE(1));
   U_NEW_ULIB_OBJECT(str_close,                 U_STRING_FROM_STRINGREP_STORAGE(2));
   U_NEW_ULIB_OBJECT(str_cookie,                U_STRING_FROM_STRINGREP_STORAGE(3));
   U_NEW_ULIB_OBJECT(str_setcookie,             U_STRING_FROM_STRINGREP_STORAGE(4));
   U_NEW_ULIB_OBJECT(str_starttls,              U_STRING_FROM_STRINGREP_STORAGE(5));
   U_NEW_ULIB_OBJECT(str_location,              U_STRING_FROM_STRINGREP_STORAGE(6));
   U_NEW_ULIB_OBJECT(str_connection,            U_STRING_FROM_STRINGREP_STORAGE(7));
   U_NEW_ULIB_OBJECT(str_user_agent,            U_STRING_FROM_STRINGREP_STORAGE(8));
   U_NEW_ULIB_OBJECT(str_authorization,         U_STRING_FROM_STRINGREP_STORAGE(9));
   U_NEW_ULIB_OBJECT(str_content_type,          U_STRING_FROM_STRINGREP_STORAGE(10));
   U_NEW_ULIB_OBJECT(str_content_length,        U_STRING_FROM_STRINGREP_STORAGE(11));
   U_NEW_ULIB_OBJECT(str_content_disposition,   U_STRING_FROM_STRINGREP_STORAGE(12));
   U_NEW_ULIB_OBJECT(str_accept_language,       U_STRING_FROM_STRINGREP_STORAGE(13));
   U_NEW_ULIB_OBJECT(str_accept_encoding,       U_STRING_FROM_STRINGREP_STORAGE(14));
   U_NEW_ULIB_OBJECT(str_if_range,              U_STRING_FROM_STRINGREP_STORAGE(15));
   U_NEW_ULIB_OBJECT(str_if_none_match,         U_STRING_FROM_STRINGREP_STORAGE(16));
   U_NEW_ULIB_OBJECT(str_if_modified_since,     U_STRING_FROM_STRINGREP_STORAGE(17));
   U_NEW_ULIB_OBJECT(str_if_unmodified_since,   U_STRING_FROM_STRINGREP_STORAGE(18));
   U_NEW_ULIB_OBJECT(str_referer,               U_STRING_FROM_STRINGREP_STORAGE(19));
   U_NEW_ULIB_OBJECT(str_X_Forwarded_For,       U_STRING_FROM_STRINGREP_STORAGE(20));
}

void USocket::close()
{
   U_TRACE(0, "USocket::close()")

   iState = CLOSE;

   USocket::closesocket();
}

bool USocket::checkIO(int iBytesTransferred, int iMaxBytesTransfer)
{
   U_TRACE(0, "USocket::checkIO(%d,%d)", iBytesTransferred, iMaxBytesTransfer)

   U_INTERNAL_ASSERT(iBytesTransferred <= iMaxBytesTransfer)

   if (iBytesTransferred <= 0)
      {
      checkErrno(iBytesTransferred);

#  ifdef DEBUG
      if (iState == TIMEOUT) U_INTERNAL_ASSERT((fcntl(iSockDesc,F_GETFL,0) & O_NONBLOCK) != O_NONBLOCK)
#  endif

      U_RETURN(false);
      }

   U_RETURN(true);
}

bool USocket::bind(SocketAddress& cLocal)
{
   U_TRACE(1, "USocket::bind(%p)", &cLocal)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT(isOpen())

   int result, counter = 0;

loop:
   result = U_SYSCALL(bind, "%d,%p,%d", iSockDesc, (sockaddr*)cLocal, cLocal.sizeOf());

   if (result == -1 &&
       errno  == EADDRINUSE)
      {
      if (counter++ <= 1)
         {
         UTimeVal(1L).nanosleep();

         goto loop;
         }
      }

   if (result == 0)
      {
      bLocalSet = true;

      cLocal.getPortNumber(iLocalPort);

      socklen_t slDummy = cLocal.sizeOf();

      result = U_SYSCALL(getsockname, "%d,%p,%p", iSockDesc, (sockaddr*)cLocal, &slDummy);

      U_INTERNAL_ASSERT_EQUALS(result,0)

      cLocal.getIPAddress(cLocalAddress);

      iState = LOGIN;

      U_RETURN(true);
      }

   U_RETURN(false);
}

bool USocket::bind(UIPAddress& addr, int port)
{
   U_TRACE(0, "USocket::bind(%p,%d)", &addr, port)

   SocketAddress cLocal;

   cLocal.setIPAddress(addr);
   cLocal.setPortNumber(port);

   return bind(cLocal);
}

bool USocket::bind(int port)
{
   U_TRACE(0, "USocket::bind(%d)", port)

   SocketAddress cLocal;

   cLocal.setIPAddressWildCard(bIPv6Socket);
   cLocal.setPortNumber(port);

   return bind(cLocal);
}

void USocket::setLocal()
{
   U_TRACE(1, "USocket::setLocal()")

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT(isOpen())

   SocketAddress cLocal;

   socklen_t slDummy = cLocal.sizeOf();

   if (U_SYSCALL(getsockname, "%d,%p,%p", iSockDesc, (sockaddr*)cLocal, &slDummy) == 0)
      {
      bLocalSet = true;
      cLocal.getPortNumber(iLocalPort);
      cLocal.getIPAddress(cLocalAddress);
      }
}

bool USocket::connect()
{
   U_TRACE(1, "USocket::connect()")

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT(isOpen())

   int result;
   SocketAddress cServer;

   cServer.setPortNumber(iRemotePort);
   cServer.setIPAddress(cRemoteAddress);

loop:
   result = U_SYSCALL(connect, "%d,%p,%d", iSockDesc, (sockaddr*)cServer, cServer.sizeOf());

   if (result == -1 && UInterrupt::checkForEventSignalPending()) goto loop;
   if (result ==  0)
      {
      setLocal();

      iState = CONNECT;

      U_RETURN(true);
      }

   U_RETURN(false);
}

ssize_t USocket::recv(int fd, void* buf, size_t len, int flags)
{
   U_TRACE(1, "USocket::recv(%d,%p,%lu,%d)", fd, buf, len, flags)

   U_INTERNAL_ASSERT(fd != -1)

   ssize_t n;

loop:
   n = U_SYSCALL(recv, "%d,%p,%d,%d", fd, CAST(buf), len, flags);

   if (n == -1 && UInterrupt::checkForEventSignalPending()) goto loop;
#ifdef DEBUG
   if (n  >  0) U_INTERNAL_DUMP("BytesRead(%d) = %#.*S", n, n, CAST(buf))
#endif

   U_RETURN(n);
}

int USocket::recvFrom(void* pBuffer, int iBufLength, uint32_t uiFlags, UIPAddress& cSourceIP, int& iSourcePortNumber)
{
   U_TRACE(1, "USocket::recvFrom(%p,%d,%u,%p,%p)", pBuffer, iBufLength, uiFlags, &cSourceIP, &iSourcePortNumber)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT(isOpen())

   int iBytesRead;
   SocketAddress cSource;

   socklen_t slDummy = cSource.sizeOf();

loop:
   iBytesRead = U_SYSCALL(recvfrom, "%d,%p,%d,%u,%p,%p", iSockDesc, CAST(pBuffer), iBufLength, uiFlags, (sockaddr*)cSource, &slDummy);

   if (iBytesRead == -1 && UInterrupt::checkForEventSignalPending()) goto loop;
   if (iBytesRead  >  0)
      {
      U_INTERNAL_DUMP("BytesRead(%d) = %#.*S", iBytesRead, iBytesRead, CAST(pBuffer))

      cSource.getIPAddress(cSourceIP);
      cSource.getPortNumber(iSourcePortNumber);
      }

   U_RETURN(iBytesRead);
}

int USocket::sendTo(void* pPayload, int iPayloadLength, uint32_t uiFlags, UIPAddress& cDestinationIP, int iDestinationPortNumber)
{
   U_TRACE(1, "USocket::sendTo(%p,%d,%u,%p,%d)", pPayload, iPayloadLength, uiFlags, &cDestinationIP, iDestinationPortNumber)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT(isOpen())

   int iBytesWrite;
   SocketAddress cDestination;

   cDestination.setIPAddress(cDestinationIP);
   cDestination.setPortNumber(iDestinationPortNumber);

loop:
   iBytesWrite = U_SYSCALL(sendto, "%d,%p,%d,%u,%p,%d", iSockDesc, CAST(pPayload), iPayloadLength, uiFlags, (sockaddr*)cDestination, cDestination.sizeOf());

   if (iBytesWrite == -1 && UInterrupt::checkForEventSignalPending()) goto loop;
#ifdef DEBUG
   if (iBytesWrite  >  0) U_INTERNAL_DUMP("BytesWrite(%d) = %#.*S", iBytesWrite, iBytesWrite, CAST(pPayload))
#endif

   U_RETURN(iBytesWrite);
}

/**
   Enables/disables the @c SO_TIMEOUT pseudo option.
   @c SO_TIMEOUT is not one of the options defined for Berkeley sockets, but
   was actually introduced as part of the Java API. For client sockets
   it has the same meaning as the @c SO_RCVTIMEO option, which specifies
   the maximum number of milliseconds that a blocking @c read() call will
   wait for data to arrive on the socket.

   @param timeoutMS the specified timeout value, in milliseconds. A value of zero indicates no timeout, i.e. an infinite wait.
*/

bool USocket::setTimeoutRCV(uint32_t timeoutMS)
{
   U_TRACE(1, "USocket::setTimeoutRCV(%u)", timeoutMS)

#if !defined(SO_RCVTIMEO)
   U_RETURN(false);
#endif

   // SO_RCVTIMEO is poorly documented in Winsock API, but it appears
   // to be measured as an int value in milliseconds, whereas BSD-style
   // sockets use a timeval

#if defined(__MINGW32__)
   bool result = setSockOpt(SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeoutMS, sizeof(uint32_t));
#else
   // Convert the timeout value (in milliseconds) into a timeval struct

   struct timeval timer;
   timer.tv_sec  =  timeoutMS / 1000;
   timer.tv_usec = (timeoutMS % 1000) * 1000;

   bool result = setSockOpt(SOL_SOCKET, SO_RCVTIMEO, &timer, sizeof(timer));
#endif

   U_RETURN(result);
}

bool USocket::setTimeoutSND(uint32_t timeoutMS)
{
   U_TRACE(1, "USocket::setTimeoutSND(%u)", timeoutMS)

#ifndef SO_SNDTIMEO 
   U_RETURN(false);
#endif

#if defined(__MINGW32__)
   bool result = setSockOpt(SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeoutMS, sizeof(uint32_t));
#else
   // Convert the timeout value (in milliseconds) into a timeval struct

   struct timeval timer;
   timer.tv_sec  =  timeoutMS / 1000;
   timer.tv_usec = (timeoutMS % 1000) * 1000;

   bool result = setSockOpt(SOL_SOCKET, SO_SNDTIMEO, &timer, sizeof(timer));
#endif

   U_RETURN(result);
}

// VIRTUAL METHOD

void USocket::closesocket()
{
   U_TRACE(1, "USocket::closesocket()")

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT(isOpen())

#ifdef __MINGW32__
   (void) U_SYSCALL(closesocket, "%d", (SOCKET)iSockDesc);
#elif defined(DEBUG)
   if (U_SYSCALL(close, "%d", iSockDesc)) U_ERROR_SYSCALL("closesocket");
#else
   (void) U_SYSCALL(close, "%d", iSockDesc);
#endif

   iSockDesc = -1;
}

bool USocket::connectServer(const UString& server, int iServPort)
{
   U_TRACE(1, "USocket::connectServer(%.*S,%d)", U_STRING_TO_TRACE(server), iServPort)

   U_CHECK_MEMORY

   if (cRemoteAddress.setHostName(server, bIPv6Socket))
      {
      iRemotePort = iServPort;

      if (connect()) U_RETURN(true);
      }

   iState = -errno;

   U_RETURN(false);
}

bool USocket::setServer(SocketAddress& cLocal, int iBackLog)
{
   U_TRACE(1, "USocket::setServer(%p,%d)", &cLocal, iBackLog)

   U_CHECK_MEMORY

   // Avoid "Address already in use" thingie

   const int iReUseAddrFlag = 1;

   (void) setSockOpt(SOL_SOCKET, SO_REUSEADDR, &iReUseAddrFlag, sizeof(iReUseAddrFlag));

   if (bind(cLocal))
      {
      if (iBackLog) listen(iBackLog);

      U_RETURN(true);
      }

   iState = -errno;

   U_RETURN(false);
}

bool USocket::setServer(int port, int iBackLog)
{
   U_TRACE(0, "USocket::setServer(%d,%d)", port, iBackLog)

   SocketAddress cLocal;

   cLocal.setIPAddressWildCard(bIPv6Socket);
   cLocal.setPortNumber(port);

   return setServer(cLocal, iBackLog);
}

bool USocket::setServer(const UString& cLocalAddr, int port, int iBackLog)
{
   U_TRACE(0, "USocket::setServer(%.*S,%d,%d)", U_STRING_TO_TRACE(cLocalAddr), port, iBackLog)

   if (cLocalAddr.empty()) return USocket::setServer(port, iBackLog);

   UIPAddress addr;

   if (addr.setHostName(cLocalAddr, bIPv6Socket) == false) U_RETURN(false);

   SocketAddress cLocal;

   cLocal.setIPAddress(addr);
   cLocal.setPortNumber(port);

   return setServer(cLocal, iBackLog);
}

bool USocket::accept(USocket* pcNewConnection)
{
   U_TRACE(1, "USocket::accept(%p)", pcNewConnection)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT(isOpen())
   U_INTERNAL_ASSERT_POINTER(pcNewConnection)

   SocketAddress cRemote;

   socklen_t slDummy = cRemote.sizeOf();

loop:
   pcNewConnection->iSockDesc = U_SYSCALL(accept, "%d,%p,%p", iSockDesc, (sockaddr*)cRemote, &slDummy);

   if (pcNewConnection->iSockDesc == -1 && UInterrupt::checkForEventSignalPending()) goto loop;
   if (pcNewConnection->iSockDesc != -1)
      {
      pcNewConnection->iState = CONNECT;

      /* we can do this in UServer...
      pcNewConnection->bLocalSet     = true;
      pcNewConnection->iLocalPort    = iLocalPort;
      pcNewConnection->cLocalAddress = cLocalAddress;
      */

      cRemote.getPortNumber(pcNewConnection->iRemotePort);
      cRemote.getIPAddress( pcNewConnection->cRemoteAddress);

      U_INTERNAL_ASSERT_EQUALS(pcNewConnection->bIPv6Socket, (cRemoteAddress.getAddressFamily() == AF_INET6))

      U_RETURN(true);
      }

   pcNewConnection->iState = -errno;

   U_RETURN(false);
}

int USocket::send(const void* pPayload, int iPayloadLength)
{
   U_TRACE(1, "USocket::send(%p,%d)", pPayload, iPayloadLength)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT(isOpen())

   ssize_t iBytesWrite;

loop:
   iBytesWrite = U_SYSCALL(send, "%d,%p,%d,%u", iSockDesc, CAST(pPayload), iPayloadLength, 0);

   if (iBytesWrite == -1 && UInterrupt::checkForEventSignalPending()) goto loop;
#ifdef DEBUG
   if (iBytesWrite  >  0) U_INTERNAL_DUMP("BytesWrite(%d) = %#.*S", iBytesWrite, iBytesWrite, CAST(pPayload))
#endif

   U_RETURN(iBytesWrite);
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* USocket::dump(bool reset) const
{
   *UObjectIO::os << "iState                        " << iState                 << '\n'
                  << "iSockDesc                     " << iSockDesc              << '\n'
                  << "bLocalSet                     " << bLocalSet              << '\n'
                  << "iLocalPort                    " << iLocalPort             << '\n'
                  << "iRemotePort                   " << iRemotePort            << '\n'
                  << "bIPv6Socket                   " << bIPv6Socket            << '\n'
                  << "cLocalAddress   (UIPAddress   " << (void*)&cLocalAddress  << ")\n"
                  << "cRemoteAddress  (UIPAddress   " << (void*)&cRemoteAddress << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
