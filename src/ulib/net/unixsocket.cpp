// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    unixsocket.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/file.h>
#include <ulib/net/unixsocket.h>
#include <ulib/utility/interrupt.h>

socklen_t           UUnixSocket::len;
const char*         UUnixSocket::path;
union uusockaddr_un UUnixSocket::addr;

void UUnixSocket::setPath(const char* pathname)
{
   U_TRACE(0, "UUnixSocket::setPath(%S)", pathname)

   path = pathname;

   unsigned slen = strlen(pathname);

   U_INTERNAL_ASSERT_MINOR(slen, sizeof(addr.psaUnixAddr.sun_path))

// if (slen > sizeof(addr.psaUnixAddr.sun_path)) slen = sizeof(addr.psaUnixAddr.sun_path);

   (void) memset(&addr, 0, sizeof(addr));

   addr.psaUnixAddr.sun_family = AF_UNIX;

   (void) memcpy(addr.psaUnixAddr.sun_path, pathname, slen);

#ifdef __SUN_LEN
   addr.psaUnixAddr.sun_len = len = sizeof(addr.psaUnixAddr.sun_len) + slen + sizeof(addr.psaUnixAddr.sun_family) + 1;
#else
                              len =                                    slen + sizeof(addr.psaUnixAddr.sun_family) + 1;
#endif
}

// VIRTUAL METHOD

/**
 * A Unix domain "server" is created as a Unix domain socket that is bound
 * to a pathname and that has a backlog queue to listen for connection
 * requests.
 */

bool UUnixSocket::setServer(int port, int iBackLog)
{
   U_TRACE(1, "UUnixSocket::setServer(%d,%d)", port, iBackLog)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT(isClosed())
   U_INTERNAL_ASSERT_POINTER(path)

   (void) UFile::unlink(path);

   iSockDesc = U_SYSCALL(socket, "%d,%d,%d", AF_UNIX, SOCK_STREAM, 0); 

   if (U_SYSCALL(bind, "%d,%p,%d", iSockDesc, &addr.psaGeneric, len) == 0)
      {
      listen(iBackLog);

      iState     = LOGIN;
      bLocalSet  = true;
      iLocalPort = iRemotePort = port;

      U_RETURN(true);
      }

   U_RETURN(false);
}

/**
 * connecting to a Unix domain socket
 */

bool UUnixSocket::connectServer(const UString& server, int iServPort)
{
   U_TRACE(1, "UUnixSocket::connectServer(%.*S,%d)", U_STRING_TO_TRACE(server), iServPort)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT(isClosed())

   int result;

   setPath(server.c_str());

   iSockDesc = U_SYSCALL(socket, "%d,%d,%d", AF_UNIX, SOCK_STREAM, 0); 

loop:
   result = U_SYSCALL(connect, "%d,%p,%d", iSockDesc, &addr.psaGeneric, len);

   if (result == -1 && UInterrupt::checkForEventSignalPending()) goto loop;
   if (result ==  0)
      {
      iState     = CONNECT;
      bLocalSet  = true;
      iLocalPort = iRemotePort = iServPort;

      U_RETURN(true);
      }

   U_RETURN(false);
}

USocket* UUnixSocket::acceptClient(USocket* pcNewConnection)
{
   U_TRACE(1, "UUnixSocket::acceptClient(%p)", pcNewConnection)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT(isOpen())

loop:
   pcNewConnection->iSockDesc = U_SYSCALL(accept, "%d,%p,%p", iSockDesc, &addr.psaGeneric, &len);

   if (pcNewConnection->iSockDesc == -1 && UInterrupt::checkForEventSignalPending()) goto loop;
   if (pcNewConnection->iSockDesc != -1)
      {
      pcNewConnection->iState = CONNECT;

      if (USocket::isBlocking()) (void) pcNewConnection->setTimeoutRCV(USocket::req_timeout * 1000);
      }
   else
      {
      pcNewConnection->iState = -errno;
      }

   U_RETURN_POINTER(pcNewConnection,USocket);
}

int UUnixSocket::recv(void* pBuffer, int iBufLength)
{
   U_TRACE(0, "UUnixSocket::recv(%p,%d)", pBuffer, iBufLength)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT(isOpen())

   int iBytesRead;

loop:
   iBytesRead = U_SYSCALL(read, "%d,%p,%d", iSockDesc, pBuffer, iBufLength);

   if (iBytesRead == -1 && UInterrupt::checkForEventSignalPending()) goto loop;
#ifdef DEBUG
   if (iBytesRead  >  0) U_INTERNAL_DUMP("BytesRead(%d) = %#.*S", iBytesRead, iBytesRead, CAST(pBuffer))
#endif

   U_RETURN(iBytesRead);
}

int UUnixSocket::send(const void* pPayload, int iPayloadLength)
{
   U_TRACE(1, "UUnixSocket::send(%p,%d)", pPayload, iPayloadLength)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT(isOpen())

   int iBytesWrite;

loop:
   iBytesWrite = U_SYSCALL(write, "%d,%p,%d", iSockDesc, pPayload, iPayloadLength);

   if (iBytesWrite == -1 && UInterrupt::checkForEventSignalPending()) goto loop;
#ifdef DEBUG
   if (iBytesWrite  >  0) U_INTERNAL_DUMP("BytesWrite(%d) = %#.*S", iBytesWrite, iBytesWrite, pPayload)
#endif

   U_RETURN(iBytesWrite);
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UUnixSocket::dump(bool reset) const
{
   USocket::dump(false);

   *UObjectIO::os << '\n'
                  << "len                           " << len          << '\n'
                  << "addr                          " << (void*)&addr << '\n'
                  << "path                          " << path;

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
