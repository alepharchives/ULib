// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    ipaddress.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/net/ipaddress.h>
#include <ulib/container/vector.h>

#include "socket_address.cpp"

const UString* UIPAddress::str_localhost;

void UIPAddress::str_allocate()
{
   U_TRACE(0, "UIPAddress::str_allocate()")

   U_INTERNAL_ASSERT_EQUALS(str_localhost,0)

   static ustringrep stringrep_storage[] = {
      { U_STRINGREP_FROM_CONSTANT("localhost") }
   };

   U_NEW_ULIB_OBJECT(str_localhost, U_STRING_FROM_STRINGREP_STORAGE(0));
}

/* Platform specific code
 *
 * These macros allow different implementations for functionality on the
 * supported platforms.  The macros define the following tasks, details on
 * specific implementations are detailed in the specific platform support sections
 *
 * IPADDR_TO_HOST(pheDetails, pcAddress, iAddressLength, iAddressType)
 *
 * Resolves the IP Address details of the provided binary IP address. The macro variables are:
 * pheDetails     (struct hostent*) - Host details returned in this structure. If the name lookup is unresolved, must be set to 0
 * pcAddress      (pointer)         - Pointer to binary IP Address
 * iAddressLength (int)             - Size of binary IP Address
 * iAddressType   (int)             - Request an AF_INET or AF_INET6 lookup
 *
 * IPNAME_TO_HOST(pheDetails, pcHostName, iAddressType, iError)
 *
 * Resolves the IP Address details of the provided host name. The macro variables are:
 * pheDetails     (struct hostent*) - Host details returned in this structure. If the address lookup is unresolved, must be set to 0
 * pcHostName     (char*)           - String representation of the host name
 * iAddressType   (int)             - Request an AF_INET or AF_INET6 lookup
 * iError         (int)             - Error details in the operation are returned in this variable
 */

#ifdef SOLARIS
/************************************************************************/
/* IPADDR_TO_HOST is implemented using the getipnodebyaddr() function */
/* IPNAME_TO_HOST is implemented using the getipnodebyname() function */
/************************************************************************/

#  define IPADDR_TO_HOST(pheDetails, pcAddress, iAddressLength, iAddressType) \
{ int rc = 0; pheDetails = (struct hostent*) U_SYSCALL(getipnodebyaddr, "%p,%d,%d,%p", pcAddress, iAddressLength, iAddressType, &rc); }

#  define IPNAME_TO_HOST(pheDetails, pcHostName, iAddressType, iError) \
{ int rc = 0; pheDetails = (struct hostent*) U_SYSCALL(getipnodebyname, "%S,%d,%d,%p", pcHostName, iAddressType, AI_DEFAULT, &rc); iError = rc; }

#else
/***********************************************************************/
/* IPADDR_TO_HOST is implemented using the gethostbyaddr() function  */
/* IPNAME_TO_HOST is implemented using the gethostbyname2() function */
/***********************************************************************/

#  define IPADDR_TO_HOST(pheDetails, pcAddress, iAddressLength, iAddressType) \
{ pheDetails = (struct hostent*) U_SYSCALL(gethostbyaddr, "%p,%d,%d", pcAddress, iAddressLength, iAddressType); }

#  if defined(HAVE_IPV6) && !defined(__MINGW32__)
#     define IPNAME_TO_HOST(pheDetails, pcHostName, iAddressType, iError) \
         { pheDetails = (struct hostent*) U_SYSCALL(gethostbyname2, "%S,%d", pcHostName, iAddressType); iError = h_errno; }
#  else
#     define IPNAME_TO_HOST(pheDetails, pcHostName, iAddressType, iError) \
         { pheDetails = (struct hostent*) U_SYSCALL(gethostbyname, "%S", pcHostName); iError = h_errno; }
#  endif
#endif

// gcc: call is unlikely and code size would grow

void UIPAddress::setAddress(void* address, bool bIPv6)
{
   U_TRACE(1, "UIPAddress::setAddress(%p,%b)", address, bIPv6)

   U_CHECK_MEMORY

#ifdef HAVE_IPV6
   if (bIPv6)
      {
      iAddressType   = AF_INET6;
      iAddressLength = sizeof(in6_addr);

      (void) u_memcpy(pcAddress.p, address, iAddressLength);
      }
   else
#endif
      {
      iAddressType   = AF_INET;
      pcAddress.i    = *(uint32_t*)address;
      iAddressLength = sizeof(in_addr);
      }

   bHostNameUnresolved = bStrAddressUnresolved = true;

   U_INTERNAL_DUMP("addr = %u", getInAddr())
}

void UIPAddress::set(const UIPAddress& cOtherAddr)
{
   U_TRACE(1, "UIPAddress::set(%p)", &cOtherAddr)

   iAddressType   = cOtherAddr.iAddressType;
   iAddressLength = cOtherAddr.iAddressLength;

#ifdef HAVE_IPV6
   if (iAddressType == AF_INET6)
      {
      U_INTERNAL_ASSERT_EQUALS(iAddressLength, sizeof(in6_addr))

      (void) u_memcpy(pcAddress.p, cOtherAddr.pcAddress.p, sizeof(in6_addr));
      }
   else
#endif
      {
      U_INTERNAL_ASSERT_EQUALS(iAddressType, AF_INET)
      U_INTERNAL_ASSERT_EQUALS(iAddressLength, sizeof(in_addr))

      pcAddress.i = cOtherAddr.pcAddress.i;
      }

   if ((bHostNameUnresolved   = cOtherAddr.bHostNameUnresolved)   == false)                 strHostName = cOtherAddr.strHostName;
   if ((bStrAddressUnresolved = cOtherAddr.bStrAddressUnresolved) == false) (void) u_strcpy(pcStrAddress, cOtherAddr.pcStrAddress);

   U_INTERNAL_DUMP("addr = %u", getInAddr())
}

void UIPAddress::setLocalHost(bool bIPv6)
{
   U_TRACE(0, "UIPAddress::setLocalHost(%b)", bIPv6)

   if (str_localhost == 0) str_allocate();

   strHostName = *str_localhost;

#ifdef HAVE_IPV6
   if (bIPv6)
      {
      pcAddress.p[0] =
      pcAddress.p[1] = 
      pcAddress.p[2] =
      pcAddress.p[3] =
      pcAddress.p[4] =
      pcAddress.p[5] = 0;

      iAddressType   = AF_INET6;
      iAddressLength = sizeof(in6_addr);

      (void) u_strcpy(pcStrAddress, "::1");
      }
   else
#endif
      {
      pcAddress.p[0] = 127;
      pcAddress.p[1] =
      pcAddress.p[2] = 0;
      pcAddress.p[3] = 1;

      iAddressType   = AF_INET;
      iAddressLength = sizeof(in_addr);

      (void) u_strcpy(pcStrAddress, "127.0.0.1");
      }

   bHostNameUnresolved = bStrAddressUnresolved = false;
}

void UIPAddress::setAddress(const char* pcNewAddress, int iNewAddressLength)
{
   U_TRACE(1, "UIPAddress::setAddress(%S,%d)", pcNewAddress, iNewAddressLength)

   U_CHECK_MEMORY

   iAddressLength = iNewAddressLength;

   (void) u_memcpy(pcAddress.p, pcNewAddress, iAddressLength);

   U_INTERNAL_DUMP("addr = %u", getInAddr())
}

bool UIPAddress::setHostName(const UString& pcNewHostName, bool bIPv6)
{
   U_TRACE(1, "UIPAddress::setHostName(%.*S,%b)", U_STRING_TO_TRACE(pcNewHostName), bIPv6)

   U_CHECK_MEMORY

   const char* name = pcNewHostName.c_str();

   if (u_isIPAddr(bIPv6, name, pcNewHostName.size()))
      {
      struct in_addr ia;

      if (inet_aton(name, &ia) == 0) U_RETURN(false);

      setAddress(&ia, false);

      (void) strHostName.replace(pcNewHostName);

      bHostNameUnresolved = false;

      U_RETURN(true);
      }

#ifdef HAVE_GETADDRINFO
   /*
   struct addrinfo {
      int ai_flags;              // Input flags
      int ai_family;             // Protocol family for socket
      int ai_socktype;           // Socket type
      int ai_protocol;           // Protocol for socket
      socklen_t ai_addrlen;      // Length of socket address
      struct sockaddr* ai_addr;  // Socket address for socket
      char* ai_canonname;        // Canonical name for service location
      struct addrinfo* ai_next;  // Pointer to next in list
   };
   */

   // host resolution functions

   int gai_err;
   struct addrinfo hints;
   struct addrinfo* result;

   // setup hints structure

   (void) memset(&hints, 0, sizeof(hints));

   hints.ai_flags    = AI_CANONNAME;
   hints.ai_family   = (bIPv6 ? AF_INET6 : AF_INET);
   hints.ai_socktype = SOCK_STREAM;

   // get our address

   gai_err = U_SYSCALL(getaddrinfo, "%S,%p,%p,%p", name, 0, &hints, &result);

   if (gai_err)
      {
      U_WARNING("getaddrinfo() error on host \"%s\": %s", name, gai_strerror(gai_err));

      U_RETURN(false);
      }

   // copy the address into our struct

   U_INTERNAL_DUMP("result = %p ai_family = %d ai_canonname = %S ai_addr = %p ai_addrlen = %d", result,
                    result->ai_family, result->ai_canonname, result->ai_addr, result->ai_addrlen)

   SocketAddress sockadd;

   sockadd.set(result);
   sockadd.getIPAddress(*this);

   iAddressType = result->ai_family;

   (void) strHostName.replace(result->ai_canonname);

   U_SYSCALL_VOID(freeaddrinfo, "%p", result);
#else
   /*
   struct hostent {
      char*  h_name;      // official name of host
      char** h_aliases;   // alias list
      int    h_addrtype;  // host address type
      int    h_length;    // length of address
      char** h_addr_list; // list of addresses
   }
   */

   int iError;
   struct hostent* pheDetails;

   IPNAME_TO_HOST(pheDetails, name, (bIPv6 ? AF_INET6 : AF_INET), iError);

   if (pheDetails == NULL)
      {
      const char* msg[2];

      switch (iError)
         {
         case HOST_NOT_FOUND:
            msg[0] = "HOST_NOT_FOUND";
            msg[1] = "The specified host is unknown";
         break;

         case NO_ADDRESS:
            msg[0] = "NO_ADDRESS";
            msg[1] = "The requested name is valid but does not have an IP address";
         break;

         case NO_RECOVERY:
            msg[0] = "NO_RECOVERY";
            msg[1] = "A non-recoverable name server error occurred";
         break;

         case TRY_AGAIN:
            msg[0] = "TRY_AGAIN";
            msg[1] = "A temporary error occurred on an authoritative name server. Try again later";
         break;

         default:
            msg[0] = "???";
            msg[1] = "unknown error";
         }

      U_WARNING("IPNAME_TO_HOST(...) - %s (%d, %s)", msg[0], iError, msg[1]);

      U_RETURN(false);
      }

   iAddressType = pheDetails->h_addrtype;

   (void) strHostName.replace(pheDetails->h_name);

   setAddress(pheDetails->h_addr_list[0], pheDetails->h_length);
#endif

   bHostNameUnresolved   = false;
   bStrAddressUnresolved = true;

   U_RETURN(true);
}

/******************************************************************************/
/* void resolveStrAddress()                                                   */
/*                                                                            */
/* This method is used to resolve the string representation of the ip         */
/* address stored by the class, this need be performed only if the lazy       */
/* evaluation flag is set. The flag is reset at completion of the method to   */
/* indicate that the address string has been resolved - the string is         */
/* generated via a call to inet_ntop().                                       */
/******************************************************************************/

void UIPAddress::resolveStrAddress()
{
   U_TRACE(1, "UIPAddress::resolveStrAddress()")

   U_CHECK_MEMORY

   if (bStrAddressUnresolved)
      {
      const char* result = 0;

#  ifdef HAVE_INET_NTOP
      result = U_SYSCALL(inet_ntop, "%d,%p,%p,%u", iAddressType, pcAddress.p, pcStrAddress, INET6_ADDRSTRLEN);
#  else
      result = U_SYSCALL(inet_ntoa, "%p", *((struct in_addr*)pcAddress.p));

      if (result) (void) u_strcpy(pcStrAddress, result);
#  endif

      if (result) bStrAddressUnresolved = false;
      }
}

/****************************************************************************/
/* void resolveHostName()                                                   */
/*                                                                          */
/* This method is used to resolve the hostname using a reverse DNS          */
/* lookup, this need be performed only if the lazy evaluation flag is set.  */
/* The flag is reset at completion of the method to indicate that the host  */
/* name has been resolved.  The reverse DNS lookup is performed via the     */
/* IPADDR_TO_HOST() macro. If the IP details lookup                         */
/* fails, pheDetails is set to NULL and we cannot obtain the host name, we  */
/* instead use the string representation of the ip address as a hostname -  */
/* calling resolveStrAddress() first to force the evaluation of this        */
/* string.  Otherwise we set the hostname from the values returned by the   */
/* function call.                                                           */
/****************************************************************************/

void UIPAddress::resolveHostName()
{
   U_TRACE(1, "UIPAddress::resolveHostName()")

   U_CHECK_MEMORY

   if (bHostNameUnresolved)
      {
#  ifdef HAVE_GETNAMEINFO
      char hbuf[NI_MAXHOST];
      SocketAddress sockadd;

      sockadd.setIPAddress(*this);

      int gai_err = U_SYSCALL(getnameinfo, "%p,%d,%p,%d,%p,%d,%d", (const sockaddr*)sockadd, sockadd.sizeOf(), hbuf, sizeof(hbuf), 0, 0, 0);

      if (gai_err == 0) (void) strHostName.replace(hbuf);
      else
         {
         U_WARNING("getnameinfo() error: %s", gai_strerror(gai_err));

         resolveStrAddress();

         (void) strHostName.replace(pcStrAddress);
         }
#  else
      /*
      struct hostent {
         char*  h_name;      // official name of host
         char** h_aliases;   // alias list
         int    h_addrtype;  // host address type
         int    h_length;    // length of address
         char** h_addr_list; // list of addresses
      }
      */

      struct hostent* pheDetails;

      IPADDR_TO_HOST(pheDetails, pcAddress.p, iAddressLength, iAddressType);

      if (pheDetails) (void) strHostName.replace(pheDetails->h_name);
      else
         {
         resolveStrAddress();

         (void) strHostName.replace(pcStrAddress);
         }
#  endif

      bHostNameUnresolved = false;
      }
}

/********************************************************************************/
/* This method converts the IPAddress instance to the specified type - either   */
/* AF_INET or AF_INET6. If the address family is already of the specified       */
/* type, then no changes are made. The following steps are for converting to:   */
/*                                                                              */
/* IPv4: If the existing IPv6 address is not an IPv4 Mapped IPv6 address the    */
/*       conversion cannot take place. Otherwise,                               */
/*       the last 32 bits of the IPv6 address form the IPv4 address and we      */
/*       call setAddress() to set the address to these four bytes.              */
/*                                                                              */
/* IPv6: The 32 bits of the IPv4 address are copied to the last 32 bits of      */
/*       the 128-bit IPv address.  This is then prepended with 80 zero bits     */
/*       and 16 one bits to form an IPv4 Mapped IPv6 address.                   */
/*                                                                              */
/* Finally, the new address family is set along with both lazy evaluation flags */
/********************************************************************************/

#ifdef HAVE_IPV6
void UIPAddress::convertToAddressFamily(int iNewAddressFamily)
{
   U_TRACE(1, "UIPAddress::convertToAddressFamily(%d)", iNewAddressFamily)

   U_CHECK_MEMORY

   if (iAddressType != iNewAddressFamily)
      {
      switch (iNewAddressFamily)
         {
         case AF_INET:
            {
            if (IN6_IS_ADDR_V4MAPPED(&(pcAddress.s))) setAddress(pcAddress.p + 12, sizeof(in_addr));
            }
         break;

         case AF_INET6:
            {
            iAddressLength = sizeof(in6_addr);

            (void)   memset(pcAddress.p,                0, 10);
            (void)   memset(pcAddress.p + 10,        0xff,  2);
            (void) u_memcpy(pcAddress.p + 12, pcAddress.p,  4);
            }
         break;
         }

      iAddressType = iNewAddressFamily;

      bHostNameUnresolved = bStrAddressUnresolved = true;
      }
}
#endif

// Simple IP-based access-control system
// Interpret a "HOST/BITS" IP mask specification. (Ex. 192.168.1.64/28)

bool UIPAllow::parseMask(const UString& spec)
{
   U_TRACE(1, "UIPAllow::parseMask(%.*S)", U_STRING_TO_TRACE(spec))

   /* get bit before slash */

   uint32_t    addr_len = spec.find('/');
   const char* addr_str = (addr_len == U_NOT_FOUND ? spec.c_str()
                                                   : spec.c_strndup(0, addr_len));

   /* extract and parse addr part */

   struct in_addr ia;

   int result = inet_aton(addr_str, &ia);

   U_INTERNAL_DUMP("addr_str = %.*S result = %d", (addr_len == U_NOT_FOUND ? spec.size() : addr_len), addr_str, result)

   U_INTERNAL_ASSERT(u_isIPv4Addr(addr_str, (addr_len == U_NOT_FOUND ? spec.size() : addr_len)))

   if (addr_len != U_NOT_FOUND) U_SYSCALL_VOID(free, "%p", (void*)addr_str);

   if (result == 0) U_RETURN(false);

   addr = ia.s_addr;

   in_addr_t allones = 0xffffffffUL;

   if (addr_len == U_NOT_FOUND)
      {
      mask = allones;
      }
   else
      {
      const char* mask_str = spec.c_pointer(addr_len + 1);

      /* find mask length as a number of bits */

      int mask_bits = atoi(mask_str);

      U_INTERNAL_DUMP("mask_bits = %d", mask_bits)

      if (mask_bits < 0 ||
          mask_bits > 32)
         {
         U_RETURN(false);
         }

      /* Make a network-endian mask with the top mask_bits set */

      if (mask_bits == 32) mask = allones;
      else                 mask = htonl(~(allones >> mask_bits));

      U_INTERNAL_DUMP("mask = %B", mask)
      }

   U_INTERNAL_DUMP("addr = %u mask = %x", addr, mask)

   U_RETURN(true);
}

uint32_t UIPAllow::parseMask(const UString& vspec, UVector<UIPAllow*>& vipallow)
{
   U_TRACE(0, "UIPAllow::parseMask(%.*S,%p)", U_STRING_TO_TRACE(vspec), &vipallow)

   UIPAllow* elem;
   UVector<UString> vec(vspec, ", ");
   uint32_t result, n = vipallow.size();

   for (uint32_t i = 0, vlen = vec.size(); i < vlen; ++i)
      {
      elem = U_NEW(UIPAllow);

      if (elem->parseMask(vec[i])) vipallow.push_back(elem);
      else                         delete elem;
      }

   result = (vipallow.size() - n);

   U_RETURN(result);
}

__pure uint32_t UIPAllow::contains(in_addr_t client, UVector<UIPAllow*>& vipallow)
{
   U_TRACE(0, "UIPAllow::contains(%u,%p)", client, &vipallow)

   for (uint32_t i = 0, vlen = vipallow.size(); i < vlen; ++i)
      {
      if (vipallow[i]->isAllowed(client)) U_RETURN(i);
      }

   U_RETURN(U_NOT_FOUND);
}

__pure uint32_t UIPAllow::contains(const UString& ip_client, UVector<UIPAllow*>& vipallow)
{
   U_TRACE(0, "UIPAllow::contains(%.*S,%p)", U_STRING_TO_TRACE(ip_client), &vipallow)

   for (uint32_t i = 0, vlen = vipallow.size(); i < vlen; ++i)
      {
      if (vipallow[i]->isAllowed(ip_client)) U_RETURN(i);
      }

   U_RETURN(U_NOT_FOUND);
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UIPAllow::dump(bool reset) const
{
   *UObjectIO::os << "addr " << addr << '\n'
                  << "mask " << mask << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

const char* UIPAddress::dump(bool reset) const
{
   *UObjectIO::os << "pcAddress            " << (void*)pcAddress.p << '\n'
                  << "pcStrAddress         ";

   char buffer[128];

   UObjectIO::os->write(buffer, u_snprintf(buffer, sizeof(buffer), "%S", pcStrAddress));

   *UObjectIO::os << '\n'
                  << "iAddressType         " << iAddressType
                                             << " (" << (iAddressType == AF_INET6 ? "IPv6" : "IPv4")
                                             << ")\n"
                  << "iAddressLength       " << iAddressLength      << '\n'
                  << "strHostName (UString " << (void*)&strHostName << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
