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

   unsigned slen = u_str_len(pathname);

   U_INTERNAL_ASSERT_MINOR(slen, sizeof(addr.psaUnixAddr.sun_path))

// if (slen > sizeof(addr.psaUnixAddr.sun_path)) slen = sizeof(addr.psaUnixAddr.sun_path);

   (void) memset(&addr, 0, sizeof(addr));

   addr.psaUnixAddr.sun_family = AF_UNIX;

   (void) u_mem_cpy(addr.psaUnixAddr.sun_path, pathname, slen);

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

   int result;

   if (isClosed())
      {
      if (path == 0) setPath(server.c_str());

      iSockDesc = U_SYSCALL(socket, "%d,%d,%d", AF_UNIX, SOCK_STREAM, 0); 
      }

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
