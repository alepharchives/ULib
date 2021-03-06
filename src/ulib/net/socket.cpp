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

#include <ulib/file.h>
#include <ulib/timeval.h>
#include <ulib/notifier.h>
#include <ulib/net/socket.h>
#include <ulib/utility/interrupt.h>
#include <ulib/utility/string_ext.h>

#include <errno.h>

int  USocket::accept4_flags;  // If flags is 0, then accept4() is the same as accept()

const UString* USocket::str_host;
const UString* USocket::str_range;
const UString* USocket::str_close;
const UString* USocket::str_cookie;
const UString* USocket::str_setcookie;
const UString* USocket::str_starttls;
const UString* USocket::str_location;
const UString* USocket::str_refresh;
const UString* USocket::str_connection;
const UString* USocket::str_user_agent;
const UString* USocket::str_authorization;
const UString* USocket::str_content_type;
const UString* USocket::str_content_length;
const UString* USocket::str_content_disposition;
const UString* USocket::str_accept;
const UString* USocket::str_accept_language;
const UString* USocket::str_accept_encoding;
const UString* USocket::str_if_range;
const UString* USocket::str_if_none_match;
const UString* USocket::str_if_modified_since;
const UString* USocket::str_if_unmodified_since;
const UString* USocket::str_referer;
const UString* USocket::str_X_Real_IP;
const UString* USocket::str_X_Forwarded_For;
const UString* USocket::str_Transfer_Encoding;
const UString* USocket::str_X_Progress_ID;
const UString* USocket::str_expect_100_continue;
const UString* USocket::str_chunked;

#include "socket_address.cpp"

void USocket::str_allocate()
{
   U_TRACE(0+256, "USocket::str_allocate()")

   U_INTERNAL_ASSERT_EQUALS(str_host,0)
   U_INTERNAL_ASSERT_EQUALS(str_range,0)
   U_INTERNAL_ASSERT_EQUALS(str_close,0)
   U_INTERNAL_ASSERT_EQUALS(str_cookie,0)
   U_INTERNAL_ASSERT_EQUALS(str_setcookie,0)
   U_INTERNAL_ASSERT_EQUALS(str_starttls,0)
   U_INTERNAL_ASSERT_EQUALS(str_location,0)
   U_INTERNAL_ASSERT_EQUALS(str_refresh,0)
   U_INTERNAL_ASSERT_EQUALS(str_connection,0)
   U_INTERNAL_ASSERT_EQUALS(str_user_agent,0)
   U_INTERNAL_ASSERT_EQUALS(str_authorization,0)
   U_INTERNAL_ASSERT_EQUALS(str_content_type,0)
   U_INTERNAL_ASSERT_EQUALS(str_content_length,0)
   U_INTERNAL_ASSERT_EQUALS(str_content_disposition,0)
   U_INTERNAL_ASSERT_EQUALS(str_accept,0)
   U_INTERNAL_ASSERT_EQUALS(str_accept_language,0)
   U_INTERNAL_ASSERT_EQUALS(str_accept_encoding,0)
   U_INTERNAL_ASSERT_EQUALS(str_if_range,0)
   U_INTERNAL_ASSERT_EQUALS(str_if_none_match,0)
   U_INTERNAL_ASSERT_EQUALS(str_if_modified_since,0)
   U_INTERNAL_ASSERT_EQUALS(str_if_unmodified_since,0)
   U_INTERNAL_ASSERT_EQUALS(str_referer,0)
   U_INTERNAL_ASSERT_EQUALS(str_X_Real_IP,0)
   U_INTERNAL_ASSERT_EQUALS(str_X_Forwarded_For,0)
   U_INTERNAL_ASSERT_EQUALS(str_Transfer_Encoding,0)
   U_INTERNAL_ASSERT_EQUALS(str_X_Progress_ID,0)
   U_INTERNAL_ASSERT_EQUALS(str_expect_100_continue,0)
   U_INTERNAL_ASSERT_EQUALS(str_chunked,0)

   static ustringrep stringrep_storage[] = {
      { U_STRINGREP_FROM_CONSTANT("Host") },
      { U_STRINGREP_FROM_CONSTANT("Range") },
      { U_STRINGREP_FROM_CONSTANT("close") },
      { U_STRINGREP_FROM_CONSTANT("Cookie") },
      { U_STRINGREP_FROM_CONSTANT("Set-Cookie") },
      { U_STRINGREP_FROM_CONSTANT("STARTTLS") },
      { U_STRINGREP_FROM_CONSTANT("Location") },
      { U_STRINGREP_FROM_CONSTANT("Refresh") },
      { U_STRINGREP_FROM_CONSTANT("Connection") },
      { U_STRINGREP_FROM_CONSTANT("User-Agent") },
      { U_STRINGREP_FROM_CONSTANT("Authorization") },
      { U_STRINGREP_FROM_CONSTANT("Content-Type") },
      { U_STRINGREP_FROM_CONSTANT("Content-Length") },
      { U_STRINGREP_FROM_CONSTANT("Content-Disposition") },
      { U_STRINGREP_FROM_CONSTANT("Accept") },
      { U_STRINGREP_FROM_CONSTANT("Accept-Language") },
      { U_STRINGREP_FROM_CONSTANT("Accept-Encoding") },
      { U_STRINGREP_FROM_CONSTANT("If-Range") },
      { U_STRINGREP_FROM_CONSTANT("If-None-Match") },
      { U_STRINGREP_FROM_CONSTANT("If-Modified-Since") },
      { U_STRINGREP_FROM_CONSTANT("If-Unmodified-Since") },
      { U_STRINGREP_FROM_CONSTANT("Referer") },
      { U_STRINGREP_FROM_CONSTANT("X-Real-IP") },
      { U_STRINGREP_FROM_CONSTANT("X-Forwarded-For") },
      { U_STRINGREP_FROM_CONSTANT("Transfer-Encoding") },
      { U_STRINGREP_FROM_CONSTANT("X-Progress-ID") },
      { U_STRINGREP_FROM_CONSTANT("Expect: 100-continue") },
      { U_STRINGREP_FROM_CONSTANT("chunked") }
   };

   U_NEW_ULIB_OBJECT(str_host,                  U_STRING_FROM_STRINGREP_STORAGE(0));
   U_NEW_ULIB_OBJECT(str_range,                 U_STRING_FROM_STRINGREP_STORAGE(1));
   U_NEW_ULIB_OBJECT(str_close,                 U_STRING_FROM_STRINGREP_STORAGE(2));
   U_NEW_ULIB_OBJECT(str_cookie,                U_STRING_FROM_STRINGREP_STORAGE(3));
   U_NEW_ULIB_OBJECT(str_setcookie,             U_STRING_FROM_STRINGREP_STORAGE(4));
   U_NEW_ULIB_OBJECT(str_starttls,              U_STRING_FROM_STRINGREP_STORAGE(5));
   U_NEW_ULIB_OBJECT(str_location,              U_STRING_FROM_STRINGREP_STORAGE(6));
   U_NEW_ULIB_OBJECT(str_refresh,               U_STRING_FROM_STRINGREP_STORAGE(7));
   U_NEW_ULIB_OBJECT(str_connection,            U_STRING_FROM_STRINGREP_STORAGE(8));
   U_NEW_ULIB_OBJECT(str_user_agent,            U_STRING_FROM_STRINGREP_STORAGE(9));
   U_NEW_ULIB_OBJECT(str_authorization,         U_STRING_FROM_STRINGREP_STORAGE(10));
   U_NEW_ULIB_OBJECT(str_content_type,          U_STRING_FROM_STRINGREP_STORAGE(11));
   U_NEW_ULIB_OBJECT(str_content_length,        U_STRING_FROM_STRINGREP_STORAGE(12));
   U_NEW_ULIB_OBJECT(str_content_disposition,   U_STRING_FROM_STRINGREP_STORAGE(13));
   U_NEW_ULIB_OBJECT(str_accept,                U_STRING_FROM_STRINGREP_STORAGE(14));
   U_NEW_ULIB_OBJECT(str_accept_language,       U_STRING_FROM_STRINGREP_STORAGE(15));
   U_NEW_ULIB_OBJECT(str_accept_encoding,       U_STRING_FROM_STRINGREP_STORAGE(16));
   U_NEW_ULIB_OBJECT(str_if_range,              U_STRING_FROM_STRINGREP_STORAGE(17));
   U_NEW_ULIB_OBJECT(str_if_none_match,         U_STRING_FROM_STRINGREP_STORAGE(18));
   U_NEW_ULIB_OBJECT(str_if_modified_since,     U_STRING_FROM_STRINGREP_STORAGE(19));
   U_NEW_ULIB_OBJECT(str_if_unmodified_since,   U_STRING_FROM_STRINGREP_STORAGE(20));
   U_NEW_ULIB_OBJECT(str_referer,               U_STRING_FROM_STRINGREP_STORAGE(21));
   U_NEW_ULIB_OBJECT(str_X_Real_IP,             U_STRING_FROM_STRINGREP_STORAGE(22));
   U_NEW_ULIB_OBJECT(str_X_Forwarded_For,       U_STRING_FROM_STRINGREP_STORAGE(23));
   U_NEW_ULIB_OBJECT(str_Transfer_Encoding,     U_STRING_FROM_STRINGREP_STORAGE(24));
   U_NEW_ULIB_OBJECT(str_X_Progress_ID,         U_STRING_FROM_STRINGREP_STORAGE(25));
   U_NEW_ULIB_OBJECT(str_expect_100_continue,   U_STRING_FROM_STRINGREP_STORAGE(26));
   U_NEW_ULIB_OBJECT(str_chunked,               U_STRING_FROM_STRINGREP_STORAGE(27));
}

USocket::USocket(bool bSocketIsIPv6)
{
   U_TRACE_REGISTER_OBJECT(0, USocket, "%b", bSocketIsIPv6)

   flags       = O_RDWR;
   iState      = CLOSE;
   iSockDesc   = -1;
   iRemotePort = 0;

#ifdef ENABLE_IPV6
   U_socket_IPv6(this) = bSocketIsIPv6;
#else
   U_socket_IPv6(this) = false;
#endif

   U_socket_LocalSet(this) = false;

#ifdef __MINGW32__
   fh = -1;
#endif

   if (str_host == 0) str_allocate();

   U_INTERNAL_DUMP("this = %p memory._this = %p iSockDesc = %d", this, memory._this, iSockDesc)
}

USocket::~USocket()
{
   U_TRACE_UNREGISTER_OBJECT(0, USocket)

   U_INTERNAL_DUMP("iSockDesc = %d", iSockDesc)

   if (isOpen()) USocket::_closesocket();
}

__pure int USocket::localPortNumber()
{
   U_TRACE(0, "USocket::localPortNumber()")

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT(isLocalSet())

   U_RETURN(iLocalPort);
}

__pure UIPAddress& USocket::localIPAddress()
{
   U_TRACE(0, "USocket::localIPAddress()")

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT(isLocalSet())

   return cLocalAddress;
}

bool USocket::socket(int iSocketType, int protocol)
{
   U_TRACE(1, "USocket::socket(%d,%d)", iSocketType, protocol)

   U_INTERNAL_ASSERT(isClosed())

#ifdef __MINGW32__
   fh        = U_SYSCALL(socket, "%d,%d,%d", U_socket_IPv6(this) ? AF_INET6 : AF_INET, iSocketType, protocol);
   iSockDesc = _open_osfhandle((long)fh, O_RDWR | O_BINARY);
#else
   iSockDesc = U_SYSCALL(socket, "%d,%d,%d", U_socket_IPv6(this) ? AF_INET6 : AF_INET, iSocketType, protocol);
#endif

   if (isOpen()) U_RETURN(true);

   U_RETURN(false);
}

bool USocket::connectServer(const UIPAddress& cAddr, int iServPort)
{
   U_TRACE(1, "USocket::connectServer(%p,%d)", &cAddr, iServPort)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT(isOpen())

   iRemotePort    = iServPort;
   cRemoteAddress = cAddr;

   if (connect()) U_RETURN(true);

   U_RETURN(false);
}

int USocket::recv(void* pBuffer, uint32_t iBufLength)
{
   U_TRACE(0, "USocket::recv(%p,%u)", pBuffer, iBufLength)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT(isOpen())

   int iBytesRead = USocket::recv(getFd(), CAST(pBuffer), iBufLength, 0);

   U_RETURN(iBytesRead);
}

void USocket::checkErrno()
{
   U_TRACE(0, "USocket::checkErrno()")

   U_INTERNAL_DUMP("errno = %d", errno)

   if (errno == EAGAIN) iState = TIMEOUT;
   else
      {
      iState = (errno == ECONNRESET ? EPOLLERROR : BROKEN);

      closesocket();
      }

   U_INTERNAL_DUMP("state = %d", iState)
}

bool USocket::checkTime(long time_limit, long& timeout)
{
   U_TRACE(1, "USocket::checkTime(%ld,%ld)", time_limit, timeout)

   U_INTERNAL_ASSERT_RANGE(1,time_limit,8L*60L) // 8 minuts

   U_gettimeofday; // NB: optimization if it is enough a time resolution of one second...

   if (timeout == 0) timeout = u_now->tv_sec + time_limit;

   if (u_now->tv_sec > timeout)
      {
      iState = USocket::BROKEN | USocket::TIMEOUT;

      close();

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

   cLocal.getPortNumber(iLocalPort);

loop:
   result = U_SYSCALL(bind, "%d,%p,%d", getFd(), (sockaddr*)cLocal, cLocal.sizeOf());

   if (result == -1         &&
       errno  == EADDRINUSE &&
       ++counter <= 3)
      {
      UTimeVal(1L).nanosleep();

      goto loop;
      }

   if (result == 0)
      {
      U_socket_LocalSet(this) = true;

      socklen_t slDummy = cLocal.sizeOf();

      result = U_SYSCALL(getsockname, "%d,%p,%p", getFd(), (sockaddr*)cLocal, &slDummy);

      U_INTERNAL_ASSERT_EQUALS(result, 0)

      cLocal.getIPAddress(cLocalAddress);

      iState = LOGIN;

      U_RETURN(true);
      }

   if (errno == EADDRINUSE)
      {
      U_WARNING("Probably another instance of userver is running on the same port: %d", iLocalPort);
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

   cLocal.setIPAddressWildCard(U_socket_IPv6(this));
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

   if (U_SYSCALL(getsockname, "%d,%p,%p", getFd(), (sockaddr*)cLocal, &slDummy) == 0)
      {
      U_socket_LocalSet(this) = true;

      cLocal.getPortNumber(iLocalPort);
      cLocal.getIPAddress(cLocalAddress);
      }
}

void USocket::setRemote()
{
   U_TRACE(1, "USocket::setRemote()")

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT(isOpen())

   SocketAddress cRemote;

   socklen_t slDummy = cRemote.sizeOf();

   if (U_SYSCALL(getpeername, "%d,%p,%p", getFd(), (sockaddr*)cRemote, &slDummy) == 0)
      {
      cRemote.getPortNumber(iRemotePort);
      cRemote.getIPAddress(cRemoteAddress);
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
   result = U_SYSCALL(connect, "%d,%p,%d", getFd(), (sockaddr*)cServer, cServer.sizeOf());

   if (result == -1 && UInterrupt::checkForEventSignalPending()) goto loop;
   if (result ==  0)
      {
      setLocal();

      iState = CONNECT;

      U_RETURN(true);
      }

   U_RETURN(false);
}

int USocket::recv(int fd, void* _buf, uint32_t len, int recv_flags)
{
   U_TRACE(1, "USocket::recv(%d,%p,%u,%d)", fd, _buf, len, recv_flags)

   U_INTERNAL_ASSERT(fd != -1)

   int n;

loop:
   n = U_SYSCALL(recv, "%d,%p,%u,%d", fd, CAST(_buf), len, recv_flags);

   if (n == -1 && UInterrupt::checkForEventSignalPending()) goto loop;
#ifdef DEBUG
   if (n  >  0) U_INTERNAL_DUMP("BytesRead(%d) = %#.*S", n, n, CAST(_buf))
#endif

   U_RETURN(n);
}

int USocket::recvFrom(void* pBuffer, uint32_t iBufLength, uint32_t uiFlags, UIPAddress& cSourceIP, int& iSourcePortNumber)
{
   U_TRACE(1, "USocket::recvFrom(%p,%u,%u,%p,%p)", pBuffer, iBufLength, uiFlags, &cSourceIP, &iSourcePortNumber)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT(isOpen())

   int iBytesRead;
   SocketAddress cSource;

   socklen_t slDummy = cSource.sizeOf();

loop:
   iBytesRead = U_SYSCALL(recvfrom, "%d,%p,%u,%u,%p,%p", getFd(), CAST(pBuffer), iBufLength, uiFlags, (sockaddr*)cSource, &slDummy);

   if (iBytesRead == -1 && UInterrupt::checkForEventSignalPending()) goto loop;
   if (iBytesRead  >  0)
      {
      U_INTERNAL_DUMP("BytesRead(%d) = %#.*S", iBytesRead, iBytesRead, CAST(pBuffer))

      cSource.getIPAddress(cSourceIP);
      cSource.getPortNumber(iSourcePortNumber);
      }

   U_RETURN(iBytesRead);
}

int USocket::sendTo(void* pPayload, uint32_t iPayloadLength, uint32_t uiFlags, UIPAddress& cDestinationIP, int iDestinationPortNumber)
{
   U_TRACE(1, "USocket::sendTo(%p,%u,%u,%p,%d)", pPayload, iPayloadLength, uiFlags, &cDestinationIP, iDestinationPortNumber)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT(isOpen())

   int iBytesWrite;
   SocketAddress cDestination;

   cDestination.setIPAddress(cDestinationIP);
   cDestination.setPortNumber(iDestinationPortNumber);

loop:
   iBytesWrite = U_SYSCALL(sendto, "%d,%p,%u,%u,%p,%d", getFd(), CAST(pPayload), iPayloadLength, uiFlags, (sockaddr*)cDestination, cDestination.sizeOf());

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

   U_INTERNAL_ASSERT_MAJOR(timeoutMS, 500) // settare un timeout a meno di mezzo secondo e' molto sospetto...

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

   U_INTERNAL_ASSERT_MAJOR(timeoutMS, 500) // settare un timeout a meno di mezzo secondo e' molto sospetto...

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

int USocket::recvBinary16Bits()
{
   U_TRACE(0, "USocket::recvBinary16Bits()")

   uint16_t uiNetOrder;
   uint32_t iBytesLeft = sizeof(uint16_t);
   char* pcEndReadBuffer = ((char*)&uiNetOrder) + iBytesLeft;

   do {
      iBytesLeft -= recv((void*)(pcEndReadBuffer - iBytesLeft), iBytesLeft);
      }
   while (iBytesLeft);

   int result = ntohs(uiNetOrder);

   U_RETURN(result);
}

uint32_t USocket::recvBinary32Bits()
{
   U_TRACE(0, "USocket::recvBinary32Bits()")

   uint32_t uiNetOrder, iBytesLeft = sizeof(uint32_t);
   char* pcEndReadBuffer = ((char*)&uiNetOrder) + iBytesLeft;

   do {
      iBytesLeft -= recv((void*)(pcEndReadBuffer - iBytesLeft), iBytesLeft);
      }
   while (iBytesLeft);

   int result = ntohl(uiNetOrder);

   U_RETURN(result);
}

bool USocket::sendBinary16Bits(uint16_t iData)
{
   U_TRACE(0, "USocket::sendBinary16Bits(%u)", iData)

   uint16_t uiNetOrder = htons(iData);

   bool result = (send((const char*)&uiNetOrder, sizeof(uint16_t)) == sizeof(uint16_t));

   U_RETURN(result);
}

bool USocket::sendBinary32Bits(uint32_t lData)
{
   U_TRACE(0, "USocket::sendBinary32Bits(%u)", lData)

   uint32_t uiNetOrder = htonl(lData);

   bool result = (send((const char*)&uiNetOrder, sizeof(uint32_t)) == sizeof(uint32_t));

   U_RETURN(result);
}

#ifdef closesocket
#undef closesocket
#endif

void USocket::_closesocket()
{
   U_TRACE(1, "USocket::_closesocket()")

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT(isOpen())

#ifdef __MINGW32__
   (void) U_SYSCALL(closesocket, "%d", fh);
                                       fh = -1;
#elif defined(DEBUG)
   if (U_SYSCALL(   close, "%d", iSockDesc)) U_ERROR_SYSCALL("closesocket");
#else
   (void) U_SYSCALL(close, "%d", iSockDesc);
#endif

   iSockDesc   = -1;
   iRemotePort = 0;
}

// VIRTUAL METHOD

const char* USocket::getMsgError(char* buffer, uint32_t buffer_size)
{
   U_TRACE(0, "USocket::getMsgError(%p,%u)", buffer, buffer_size)

   U_INTERNAL_DUMP("iState = %d", iState)

   if (isSysError())
      {
      errno = -iState;

      (void) u__snprintf(buffer, buffer_size, "%R", 0); // NB: the last argument (0) is necessary...

      buffer += 3;

      U_RETURN((const char*)buffer);
      }

   U_RETURN((const char*)0);
}

bool USocket::connectServer(const UString& server, int iServPort)
{
   U_TRACE(1, "USocket::connectServer(%.*S,%d)", U_STRING_TO_TRACE(server), iServPort)

   U_CHECK_MEMORY

   if (cRemoteAddress.setHostName(server, U_socket_IPv6(this)))
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

   const int _flag = 1;
   struct linger ling = { 0, 0 };

   (void) setSockOpt(SOL_SOCKET, SO_REUSEADDR, &_flag, sizeof(_flag));
   (void) setSockOpt(SOL_SOCKET, SO_KEEPALIVE, &_flag, sizeof(_flag));
   (void) setSockOpt(SOL_SOCKET, SO_LINGER,    &ling,  sizeof(ling));
#ifdef SO_REUSEPORT
   (void) setSockOpt(SOL_SOCKET, SO_REUSEPORT, &_flag, sizeof(_flag)); // http://git.kernel.org/linus/c617f398edd4db2b8567a28e899a88f8f574798d
#endif

   if (bind(cLocal))
      {
      listen(iBackLog);

      U_RETURN(true);
      }

   iState = -errno;

   U_RETURN(false);
}

bool USocket::setServer(int port, int iBackLog)
{
   U_TRACE(0, "USocket::setServer(%d,%d)", port, iBackLog)

   SocketAddress cLocal;

   cLocal.setIPAddressWildCard(U_socket_IPv6(this));
   cLocal.setPortNumber(port);

   return setServer(cLocal, iBackLog);
}

bool USocket::setServer(const UString& cLocalAddr, int port, int iBackLog)
{
   U_TRACE(0, "USocket::setServer(%.*S,%d,%d)", U_STRING_TO_TRACE(cLocalAddr), port, iBackLog)

   if (cLocalAddr.empty()) return USocket::setServer(port, iBackLog);

   UIPAddress addr;

   if (addr.setHostName(cLocalAddr, U_socket_IPv6(this)) == false) U_RETURN(false);

   SocketAddress cLocal;

   cLocal.setIPAddress(addr);
   cLocal.setPortNumber(port);

   return setServer(cLocal, iBackLog);
}

bool USocket::acceptClient(USocket* pcNewConnection)
{
   U_TRACE(1, "USocket::acceptClient(%p)", pcNewConnection)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT(isOpen())
   U_INTERNAL_ASSERT_POINTER(pcNewConnection)

   SocketAddress cRemote;

   socklen_t slDummy = cRemote.sizeOf();

#ifdef HAVE_ACCEPT4
       pcNewConnection->iSockDesc  = U_SYSCALL(accept4, "%d,%p,%p,%d", iSockDesc, (sockaddr*)cRemote, &slDummy, accept4_flags);
// if (pcNewConnection->iSockDesc != -1 || errno != ENOSYS) goto next;
#elif defined(__MINGW32__)
   pcNewConnection->fh        = U_SYSCALL(accept, "%d,%p,%p", fh, (sockaddr*)cRemote, &slDummy);
   pcNewConnection->iSockDesc = _open_osfhandle((long)(pcNewConnection->fh), O_RDWR | O_BINARY);
#else
   pcNewConnection->iSockDesc = U_SYSCALL(accept, "%d,%p,%p", iSockDesc, (sockaddr*)cRemote, &slDummy);
#endif
// next:
   if (pcNewConnection->iSockDesc != -1)
      {
      pcNewConnection->iState = CONNECT;

      cRemote.getPortNumber(pcNewConnection->iRemotePort);
      cRemote.getIPAddress( pcNewConnection->cRemoteAddress);

      U_INTERNAL_DUMP("pcNewConnection->iSockDesc = %d pcNewConnection->flags = %d %B",
                       pcNewConnection->iSockDesc, pcNewConnection->flags, pcNewConnection->flags)

      U_INTERNAL_ASSERT_EQUALS(U_socket_IPv6(pcNewConnection), (cRemoteAddress.getAddressFamily() == AF_INET6))
      
#  ifdef HAVE_ACCEPT4
      U_INTERNAL_ASSERT_EQUALS(((accept4_flags & SOCK_CLOEXEC)  != 0),((pcNewConnection->flags & O_CLOEXEC)  != 0))
      U_INTERNAL_ASSERT_EQUALS(((accept4_flags & SOCK_NONBLOCK) != 0),((pcNewConnection->flags & O_NONBLOCK) != 0))
#  else
      if (accept4_flags) (void) U_SYSCALL(fcntl, "%d,%d,%d", pcNewConnection->iSockDesc, F_SETFL, pcNewConnection->flags);
#  endif

/*
#  ifdef DEBUG
      uint32_t value = U_NOT_FOUND, tmp = sizeof(uint32_t);
#     ifdef TCP_CORK
      (void) pcNewConnection->getSockOpt(SOL_TCP, TCP_CORK, (void*)&value, tmp);
      U_INTERNAL_DUMP("TCP_CORK = %d", value)
#     endif
#     ifdef TCP_DEFER_ACCEPT
      (void) pcNewConnection->getSockOpt(SOL_TCP, TCP_DEFER_ACCEPT, (void*)&value, tmp);
      U_INTERNAL_DUMP("TCP_DEFER_ACCEPT = %d", value)
#     endif
#     ifdef TCP_QUICKACK
      (void) pcNewConnection->getSockOpt(SOL_TCP, TCP_QUICKACK, (void*)&value, tmp);
      U_INTERNAL_DUMP("TCP_QUICKACK = %d", value)
#     endif
#     ifdef TCP_NODELAY
      (void) pcNewConnection->getSockOpt(SOL_TCP, TCP_NODELAY, (void*)&value, tmp);
      U_INTERNAL_DUMP("TCP_NODELAY = %d", value)
#     endif
#     ifdef TCP_FASTOPEN
      (void) pcNewConnection->getSockOpt(SOL_TCP, TCP_FASTOPEN, (void*)&value, tmp);
      U_INTERNAL_DUMP("TCP_FASTOPEN = %d", value)
#     endif
#     ifdef SO_KEEPALIVE
      (void) pcNewConnection->getSockOpt(SOL_SOCKET, SO_KEEPALIVE, (void*)&value, tmp);
      U_INTERNAL_DUMP("SO_KEEPALIVE = %d", value)
#     endif
#     ifdef TCP_CONGESTION
      char buffer[32];
      uint32_t tmp1 = sizeof(buffer);
      (void) pcNewConnection->getSockOpt(IPPROTO_TCP, TCP_CONGESTION, (void*)buffer, tmp1);
      U_INTERNAL_DUMP("TCP_CONGESTION = %S", buffer)
#     endif
      struct linger x = { 0, -1 }; // { int l_onoff; int l_linger; }
      uint32_t tmp0 = sizeof(struct linger);
      (void) pcNewConnection->getSockOpt(SOL_SOCKET, SO_LINGER, (void*)&x, tmp0);
      U_INTERNAL_DUMP("SO_LINGER = { %d %d }", x.l_onoff, x.l_linger)
      U_DUMP("getBufferRCV() = %u getBufferSND() = %u", pcNewConnection->getBufferRCV(), pcNewConnection->getBufferSND())
#  endif
*/

      U_RETURN(true);
      }

   U_INTERNAL_ASSERT_EQUALS(pcNewConnection->iSockDesc, -1)

   // NB: we never restart accept(), in general socket server is NOT blocking...

   if (errno == EINTR) (void) UInterrupt::checkForEventSignalPending();

   pcNewConnection->iState = -errno;

   U_RETURN(false);
}

int USocket::send(const char* pPayload, uint32_t iPayloadLength)
{
   U_TRACE(1, "USocket::send(%p,%u)", pPayload, iPayloadLength)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT(isOpen())

   int iBytesWrite;

loop:
   iBytesWrite = U_SYSCALL(send, "%d,%p,%u,%u", getFd(), CAST(pPayload), iPayloadLength, 0);

   if (iBytesWrite == -1 && UInterrupt::checkForEventSignalPending()) goto loop;
#ifdef DEBUG
   if (iBytesWrite  >  0) U_INTERNAL_DUMP("BytesWrite(%d) = %#.*S", iBytesWrite, iBytesWrite, CAST(pPayload))
#endif

   U_RETURN(iBytesWrite);
}

// timeoutMS specified the timeout value, in milliseconds.
// A negative value indicates no timeout, i.e. an infinite wait

int USocket::recv(void* pBuffer, uint32_t iBufLength, int timeoutMS)
{
   U_TRACE(0, "USocket::recv(%p,%u,%d)", pBuffer, iBufLength, timeoutMS)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT(isOpen())

   int iBytesRead;

   if (timeoutMS != 0 &&
       isBlocking()   &&
       UNotifier::waitForRead(iSockDesc, timeoutMS) != 1)
      {
      U_RETURN(-1);
      }

loop:
   iBytesRead = recv(pBuffer, iBufLength);

   if (iBytesRead == -1     &&
       errno      == EAGAIN &&
       timeoutMS  != 0      &&
       UNotifier::waitForRead(iSockDesc, timeoutMS) == 1)
      {
      goto loop;
      }

   U_RETURN(iBytesRead);
}

int USocket::send(const void* pPayload, uint32_t iPayloadLength, int timeoutMS)
{
   U_TRACE(1, "USocket::send(%p,%u,%d)", pPayload, iPayloadLength, timeoutMS)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT(isOpen())

   int iBytesWrite;

loop:
   iBytesWrite = send((const char*)pPayload, iPayloadLength);

   if (iBytesWrite == -1     &&
       errno       == EAGAIN &&
       timeoutMS   != 0)
      {
      int res = UNotifier::waitForWrite(iSockDesc, timeoutMS);

      if (res ==  1) goto loop;
      if (res == -1)
         {
         iState = BROKEN;

         _closesocket();
         }
      }

   U_RETURN(iBytesWrite);
}

/*
sendfile() copies data between one file descriptor and another. Either or both of these file descriptors may refer to a socket.
OUT_FD should be a descriptor opened for writing. POFFSET is a pointer to a variable holding the input file pointer position from
which sendfile() will start reading data. When sendfile() returns, this variable will be set to the offset of the byte following
the last byte that was read. COUNT is the number of bytes to copy between file descriptors. Because this copying is done within
the kernel, sendfile() does not need to spend time transferring data to and from user space.
*/

bool USocket::sendfile(int in_fd, off_t* poffset, uint32_t count, int timeoutMS)
{
   U_TRACE(1, "USocket::sendfile(%d,%p,%u,%d)", in_fd, poffset, count, timeoutMS)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT(isOpen())
   U_INTERNAL_ASSERT_MAJOR(count,0)

   ssize_t value;

   do {
      U_INTERNAL_DUMP("count = %u", count)

      value = U_SYSCALL(sendfile, "%d,%d,%p,%u", getFd(), in_fd, poffset, count);

      if (value == (ssize_t)count) U_RETURN(true);

      if (value <= 0L)
         {
         U_INTERNAL_DUMP("errno = %d", errno)

         if (errno     == EAGAIN &&
             timeoutMS != 0      &&
             UNotifier::waitForWrite(iSockDesc, timeoutMS) == 1)
            {
            continue;
            }

         iState = BROKEN;

         _closesocket();

         U_RETURN(false);
         }

      count -= value;
      }
   while (count);

   U_INTERNAL_ASSERT_EQUALS(count,0)

   U_RETURN(true);
}

// write data into multiple buffers

int USocket::_writev(const struct iovec* iov, int iovcnt)
{
   U_TRACE(0, "USocket::_writev(%p,%d)", iov, iovcnt)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT(isOpen())

   int iBytesWrite = 0, value;

   for (int i = 0; i < iovcnt; ++i)
      {
      if (iov[i].iov_len)
         {
         value = send((const char*)iov[i].iov_base, iov[i].iov_len);

         if (value <= 0) break;

         iBytesWrite += value;
         }
      }

   U_RETURN(iBytesWrite);
}

int USocket::writev(const struct iovec* _iov, int iovcnt)
{
   U_TRACE(1, "USocket::writev(%p,%d)", _iov, iovcnt)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT(isOpen())

   int iBytesWrite;
loop:
   iBytesWrite = U_SYSCALL(writev, "%d,%p,%d", iSockDesc, _iov, iovcnt);

   if (iBytesWrite == -1 && UInterrupt::checkForEventSignalPending()) goto loop;
#ifdef DEBUG
   if (iBytesWrite  >  0) U_INTERNAL_DUMP("BytesWrite(%d) = %.*S", iBytesWrite, _iov[0].iov_len, _iov[0].iov_base)
#endif

   U_RETURN(iBytesWrite);
}

int USocket::writev(const struct iovec* _iov, int iovcnt, int timeoutMS)
{
   U_TRACE(1, "USocket::writev(%p,%d,%d)", _iov, iovcnt, timeoutMS)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT(isOpen())

   int iBytesWrite;

loop:
#ifdef __MINGW32__
   iBytesWrite = _writev(_iov, iovcnt);
#else
   iBytesWrite =  writev(_iov, iovcnt);
#endif

   if (iBytesWrite == -1     &&
       errno       == EAGAIN &&
       timeoutMS   != 0)
      {
      int res = UNotifier::waitForWrite(iSockDesc, timeoutMS);

      if (res ==  1) goto loop;
      if (res == -1)
         {
         iState = BROKEN;

         _closesocket();
         }
      }

   U_RETURN(iBytesWrite);
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* USocket::dump(bool reset) const
{
   *UObjectIO::os << "flags                         " << flags                  << '\n'
                  << "iState                        " << iState                 << '\n'
                  << "iSockDesc                     " << iSockDesc              << '\n'
                  << "iLocalPort                    " << iLocalPort             << '\n'
                  << "iRemotePort                   " << iRemotePort            << '\n'
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
