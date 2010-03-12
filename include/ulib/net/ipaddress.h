// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    ipaddress.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_IPADDRES_H
#define ULIB_IPADDRES_H 1

#include <ulib/string.h>

#if defined(__MINGW32__)
#  include <ws2tcpip.h>
#else
#  include <netinet/in.h>
#  include <sys/socket.h>
#endif

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 46
#endif

/****************************************************************************/
/* This class is used to provide transparent IPv4 and IPv6 address support  */
/* for the USocket classes within the library. The constructor creates a    */
/* default IPv4 address to localhost. The address can be assigned and parts */
/* of the address can be extracted for usage. This class is intended to be  */
/* used in conjunction with the USocket classes.                            */
/****************************************************************************/

class USocket;
template <class T> class UVector;

class U_EXPORT UIPAllow {
public:

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   // COSTRUTTORI

   UIPAllow()
      {
      U_TRACE_REGISTER_OBJECT(0, UIPAllow, "", 0)
      }

   ~UIPAllow()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UIPAllow)
      }

   // ASSEGNAZIONE

   UIPAllow& operator=(const UIPAllow& a)
      {
      U_TRACE(0, "UIPAllow::operator=(%p)", &a)

      U_MEMORY_TEST_COPY(a)

      addr = a.addr;
      mask = a.mask;

      return *this;
      }

   // Interpret a "HOST/BITS" IP mask specification. (Ex. 192.168.1.64/28)

          bool     parseMask(const UString&  spec);
   static uint32_t parseMask(const UString& vspec, UVector<UIPAllow*>& vipallow);

   // Check whether a ip address client ought to be allowed

   bool isAllowed(in_addr_t client)
      {
      U_TRACE(0, "UIPAllow::isAllowed(%u)", client)

      U_INTERNAL_DUMP("addr = %u mask = %x", addr, mask)

      bool result = ((client & mask) == (addr & mask));

      U_RETURN(result);
      }

   bool isAllowed(const UString& ip_client)
      {
      U_TRACE(0, "UIPAllow::isAllowed(%.*S)", U_STRING_TO_TRACE(ip_client))

      U_INTERNAL_ASSERT(ip_client.isNullTerminated())
      U_INTERNAL_ASSERT(u_isIPv4Addr(U_STRING_TO_PARAM(ip_client)))

      struct in_addr ia;

      bool result = (inet_aton(ip_client.data(), &ia) && isAllowed(ia.s_addr));

      U_RETURN(result);
      }

   static bool isAllowed(in_addr_t         client, UVector<UIPAllow*>& vipallow);
   static bool isAllowed(const UString& ip_client, UVector<UIPAllow*>& vipallow);

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   in_addr_t addr, mask;

private:
   UIPAllow(const UIPAllow&) {}
};

union uupcAddress {
   char p[16];
   uint32_t* i;
};

class U_EXPORT UIPAddress {
public:

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   // COSTRUTTORI

   UIPAddress()
      {
      U_TRACE_REGISTER_OBJECT(0, UIPAddress, "", 0)
      }

   ~UIPAddress()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UIPAddress)
      }

   // Sets an UIPAddress by providing a host name and a
   // boolean variable to indicate whether we want an IPv6
   // or IPv4 address.

   void setLocalHost(                             bool bIPv6 = false);
   bool setHostName(const UString& pcNewHostName, bool bIPv6 = false);

   // Sets an UIPAddress by providing a pointer to an address
   // structure of the form in_addr or in6_addr. This pointer is cast to (void*).
   // A boolean value is used to indicate if this points to an IPv6 or IPv4 address.

   void setAddress(void* address, bool bIPv6 = false);

protected:

   /****************************************************************************/
   /* This method is used to set the contents of the iAddressLength and        */
   /* pcAddress member variables. Address Length bytes are copied from the     */
   /* source address to the pcAddress array. This array is 16 bytes long, long */
   /* enough to hold both IPv4 and IPv6 addresses.                             */
   /****************************************************************************/

   void setAddress(const char* pcNewAddress, int iNewAddressLength)
      {
      U_TRACE(1, "UIPAddress::setAddress(%S,%d)", pcNewAddress, iNewAddressLength)

      U_CHECK_MEMORY

      iAddressLength = iNewAddressLength;

      (void) U_SYSCALL(memcpy, "%p,%p,%u", pcAddress.p, pcNewAddress, iAddressLength);

      U_INTERNAL_DUMP("addr = %u", getInAddr())
      }

public:

   // Returns a constant integer of the address family represented by the UIPAddress.

   u_short getAddressFamily() const { return iAddressType; }

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
   void convertToAddressFamily(int iNewAddressFamily)
      {
      U_TRACE(1, "UIPAddress::convertToAddressFamily(%d)", iNewAddressFamily)

      U_CHECK_MEMORY

      if (iAddressType != iNewAddressFamily)
         {
         switch (iNewAddressFamily)
            {
            case AF_INET:
               {
               if (IN6_IS_ADDR_V4MAPPED(pcAddress.i))
                  {
                  setAddress(pcAddress.p + 12, sizeof(in_addr));
                  }
               }
            break;

            case AF_INET6:
               {
               iAddressLength = sizeof(in6_addr);

               (void) memset(pcAddress.p,               0, 10);
               (void) memset(pcAddress.p + 10,       0xff,  2);
               (void) memcpy(pcAddress.p + 12, pcAddress.p, 4);
               }
            break;
            }

         iAddressType = iNewAddressFamily;

         bHostNameUnresolved = bStrAddressUnresolved = true;
         }
      }
#endif

   // Returns the address represented by UIPAddress

   in_addr_t getInAddr() const       { return *(in_addr_t*)(pcAddress.p + (iAddressType == AF_INET6 ? 12 : 0)); }
   int       getInAddrLength() const { return iAddressLength; }

   // Returns a (void*) to the address represented by UIPAddress.
   // This must be cast to (in_addr*) or to (in6_addr*) for use

   void* get_in_addr() const { return (void*) (pcAddress.p + (iAddressType == AF_INET6 ? 12 : 0)); }

   // Returns a string of the hostname of the represented IP Address

   UString& getHostName() { resolveHostName(); return strHostName; }

   // Returns a constant string pointer to the string
   // representation of the IP Address suitable for visual presentation

   const char* getAddressString() { resolveStrAddress(); return pcStrAddress; }

   // Check equality with an existing UIPAddress object

   bool operator==(const UIPAddress& cOtherAddr) const
      {
      U_TRACE(0, "UIPAddress::operator==(%p)", &cOtherAddr)

      U_CHECK_MEMORY

      bool result = (iAddressType   == cOtherAddr.iAddressType) &&
                    (iAddressLength == cOtherAddr.iAddressLength) &&
                    (U_SYSCALL(memcmp, "%p,%p,%u", pcAddress.p, cOtherAddr.pcAddress.p, iAddressLength) == 0);

      U_RETURN(result);
      }

   bool operator!=(const UIPAddress& cOtherAddr) const { return !operator==(cOtherAddr); }

   // ASSEGNAZIONE

   void set(const UIPAddress& cOtherAddr)
      {
      U_TRACE(1, "UIPAddress::set(%p)", &cOtherAddr)

      iAddressType = cOtherAddr.iAddressType;

      (void) U_SYSCALL(memcpy, "%p,%p,%u", pcAddress.p,  cOtherAddr.pcAddress.p, (iAddressLength = cOtherAddr.iAddressLength));

      if ((bHostNameUnresolved   = cOtherAddr.bHostNameUnresolved)   == false) strHostName = cOtherAddr.strHostName;
      if ((bStrAddressUnresolved = cOtherAddr.bStrAddressUnresolved) == false) (void) strcpy(pcStrAddress, cOtherAddr.pcStrAddress);
      }

   UIPAddress(const UIPAddress& cOtherAddr)
      {
      U_TRACE_REGISTER_OBJECT(0, UIPAddress, "%p", &cOtherAddr)

      U_MEMORY_TEST_COPY(cOtherAddr)

      set(cOtherAddr);
      }

   UIPAddress& operator=(const UIPAddress& cOtherAddr)
      {
      U_TRACE(1, "UIPAddress::operator=(%p)", &cOtherAddr)

      U_MEMORY_TEST_COPY(cOtherAddr)

      set(cOtherAddr);

      return *this;
      }

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   union uupcAddress pcAddress;
   char pcStrAddress[INET6_ADDRSTRLEN];
   int iAddressLength, iAddressType;
   UString strHostName;
   bool bHostNameUnresolved, bStrAddressUnresolved;

   void resolveHostName();
   void resolveStrAddress();

private:
   friend class USocket;
};

#endif
