// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    ping.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/file.h>
#include <ulib/process.h>
#include <ulib/timeval.h>
#include <ulib/net/ping.h>
#include <ulib/container/vector.h>
#include <ulib/utility/services.h>
#include <ulib/utility/socket_ext.h>

#ifdef __MINGW32__
#  define ICMP_ECHO       8  /* Echo Request */
#  define ICMP_ECHOREPLY  0  /* Echo Reply   */
#else
#  include <netinet/ip.h>
#  include <netinet/in.h>
#  include <netinet/ip_icmp.h>
#  ifdef HAVE_NETPACKET_PACKET_H
#     include <net/if.h>
#     include <sys/ioctl.h>
#     include <arpa/inet.h>
#     include <net/if_arp.h>
#  endif
#endif

#ifndef HAVE_IPHDR
typedef struct iphdr {
   uint8_t  ip_vhl:4;   /* Length of the header in dwords */
   uint8_t  version:4;  /* Version of IP                  */
   uint8_t  tos;        /* Type of service                */
   uint16_t total_len;  /* Length of the packet in dwords */
   uint16_t ident;      /* unique identifier              */
   uint16_t flags;      /* Flags                          */
   uint8_t  ip_ttl;     /* Time to live                   */
   uint8_t  protocol;   /* Protocol number (TCP, UDP etc) */
   uint16_t checksum;   /* IP checksum                    */
   uint32_t source_ip, dest_ip;
   /* The options start here */
} iphdr;
#endif

/*
#ifndef HAVE_SOCKADDR_LL
typedef struct sockaddr_ll {
   uint16_t sll_family;    // Always AF_PACKET
   uint16_t sll_protocol;  // Physical layer protocol
   int      sll_ifindex;   // Interface number
   uint16_t sll_hatype;    // Header type
   uint8_t  sll_pkttype;   // Packet type
   uint8_t  sll_halen;     // Length of address
   uint8_t  sll_addr[8];   // Physical layer address
};
#endif
*/

/* See RFC 826 for protocol description. ARP packets are variable in size; the arphdr structure defines the fixed-length portion.
 * Protocol type values are the same as those for 10 Mb/s Ethernet. It is followed by the variable-sized fields ar_sha, arp_spa,
 * arp_tha and arp_tpa in that order, according to the lengths specified. Field names used correspond to RFC 826.

#ifndef HAVE_ARPHDR
typedef struct arphdr {
   uint16_t ar_hrd;              // Format of hardware address
   uint16_t ar_pro;              // Format of protocol address
   uint8_t  ar_hln;              // Length of hardware address
   uint8_t  ar_pln;              // Length of protocol address
   uint16_t ar_op;               // ARP opcode (command)
// ----------------------------------------------------------------
// Ethernet looks like this : This bit is variable sized however...
// ETH_ALEN == 6 (Octets in one ethernet addr)
// ----------------------------------------------------------------
// uint8_t __ar_sha[ETH_ALEN];   // Sender hardware address
// uint8_t __ar_sip[4];          // Sender IP address
// uint8_t __ar_tha[ETH_ALEN];   // Target hardware address
// uint8_t __ar_tip[4];          // Target IP address
// ----------------------------------------------------------------
} arphdr;
#endif
*/

#ifndef ETH_P_IP
#define ETH_P_IP  0x0800 /* Internet Protocol packet */
#endif
#ifndef ETH_P_ARP
#define ETH_P_ARP 0x0806 /* Address Resolution packet */
#endif

fd_set*   UPing::addrmask;
UProcess* UPing::proc;

UPing::UPing(int _timeoutMS, bool bSocketIsIPv6) : USocket(bSocketIsIPv6)
{
   U_TRACE_REGISTER_OBJECT(0, UPing, "%d,%b", _timeoutMS, bSocketIsIPv6)

   timeoutMS = _timeoutMS;

   if (addrmask == 0)
      {
      proc     = U_NEW(UProcess);
      addrmask = (fd_set*) UFile::mmap(sizeof(fd_set) + sizeof(uint32_t));
      }
}

UPing::~UPing()
{
   U_TRACE_UNREGISTER_OBJECT(0, UPing)

   if (addrmask)
      {
      delete proc;

      UFile::munmap(addrmask, sizeof(fd_set) + sizeof(uint32_t));
                    addrmask = 0;
      }
}

// Checksum (16 bits), calculated with the ICMP part of the packet (the IP header is not used)

inline void UPing::cksum(void* hdr, size_t len)
{
   U_TRACE(0, "UPing::cksum(%p,%d)", hdr, len)

   uint8_t* cb = (uint8_t*)hdr;
   size_t i = 0, b1 = 0, b2 = 0;

   ((icmphdr*)hdr)->checksum = 0;

   for (; i < (len & ~1); i += 2)
      {
      b1 += cb[i];
      b2 += cb[i + 1];
      }

   if (i & 1) b1 += cb[len - 1];

   while (true)
      {
      if (b1 >= 256)
         {
         b2 += b1 >> 8;
         b1 &= 0xff;

         continue;
         }

      if (b2 >= 256)
         {
         b1 += b2 >> 8;
         b2 &= 0xff;

         continue;
         }

      break;
      }

   cb    = (uint8_t*)&(((icmphdr*)hdr)->checksum);
   cb[0] = ~(uint8_t)b1;
   cb[1] = ~(uint8_t)b2;

   U_INTERNAL_DUMP("b1 = %d b2 = %d", b1, b2)
}

// This method is called to test whether a particular host is reachable across an IP network; it is also used to self test the network interface card
// of the computer, or as a latency test. It works by sending ICMP echo request packets to the target host and listening for ICMP echo response replies.
// Note that ICMP (and therefore ping) resides on the Network layer (level 3) of the OSI (Open Systems Interconnection) model. This is the same layer as
// IP (Internet Protocol). Consequently, ping does not use a port for communication.

void UPing::initPing()
{
   U_TRACE(0, "UPing::initPing()")

   if (USocket::socket(SOCK_RAW, IPPROTO_ICMP) == false)
      {
      if (UServices::isSetuidRoot() == false) U_ERROR("Must run as root to create raw socket...", 0);

      U_ERROR("Sorry, could not create raw socket", 0);
      }

   (void) setTimeoutSND(timeoutMS);
   (void) setTimeoutRCV(timeoutMS);
}

bool UPing::ping(UIPAddress& addr)
{
   U_TRACE(0, "UPing::ping(%p)", &addr)

   U_CHECK_MEMORY

   if (USocket::isClosed()) initPing();

   union uuiphdr {
      struct iphdr* ph;
      unsigned char* pc;
   };

   union uuiphdr u;
   UIPAddress cResponseIP;
   unsigned char buf[4096];
   size_t iphdrlen, seq = 0;
   int ret, iSourcePortNumber;

   (void) U_SYSCALL(memset, "%p,%C,%u", &req, 0, sizeof(req));

   req.id   = htons((short)u_pid); //           identifier (16 bits)
   req.type = ICMP_ECHO;           // Type of ICMP message ( 8 bits) -> 8

   for (int i = 0; i < 3; ++i)
      {
      req.seq = htons(++seq);

      // Checksum (16 bits), calculated with the ICMP part of the packet (the IP header is not used)

      cksum(&req, sizeof(req));

      U_INTERNAL_DUMP("id = %hd seq = %hd", ntohs(req.id), ntohs(req.seq))

      // send icmp echo request

      ret = sendTo((void*)&req, sizeof(req), 0, addr, 0);

      if (checkIO(ret, sizeof(req)) == false) U_RETURN(false);

      // wait for response
loop:
      ret = recvFrom(buf, sizeof(buf), 0, cResponseIP, iSourcePortNumber);

      if (ret <= 0)
         {
         checkErrno(ret);

         if (isTimeout()) continue;

         U_RETURN(false);
         }

      if (cResponseIP != addr) goto loop;

      if (bIPv6Socket)
         {
         if (ret < (int)sizeof(rephdr)) goto loop; 

         iphdrlen = 0;
         }
      else
         {
         if (ret < (int)(sizeof(struct iphdr) + sizeof(rephdr) - 4)) goto loop;

         iphdrlen = sizeof(struct iphdr);

         u.pc = buf;

         U_INTERNAL_DUMP("protocol = %d", u.ph->protocol)

         if (u.ph->protocol != IPPROTO_ICMP) goto loop; // IPPROTO_ICMP -> 1
         }

      rep = (rephdr*)(buf + iphdrlen);

      U_INTERNAL_DUMP("iphdrlen = %d type = %d id = %hd seq = %hd", iphdrlen, rep->type, ntohs(rep->id), ntohs(rep->seq))

      if ((rep->id   != req.id)  ||
          (rep->seq  != req.seq) ||
          (rep->type != ICMP_ECHOREPLY)) // ICMP_ECHOREPLY -> 0
         {
         goto loop;
         }

      U_INTERNAL_DUMP("TTL = %d", ntohl(rep->ttl))

      U_RETURN(true);
      }

   U_RETURN(false);
}

// parallel PING/ARPING

#define SHM_counter (*(uint32_t*)(((char*)addrmask)+sizeof(fd_set)))

fd_set* UPing::pingAsyncCompletion()
{
   U_TRACE(0, "UPing::pingAsyncCompletion()")

   U_INTERNAL_ASSERT_POINTER(proc)

   proc->waitAll();

   U_INTERNAL_DUMP("SHM_counter = %u addrmask = %B", SHM_counter, __FDS_BITS(addrmask)[0])

   U_RETURN_POINTER(addrmask, fd_set);
}

fd_set* UPing::checkForPingAsyncCompletion(uint32_t nfds)
{
   U_TRACE(0, "UPing::checkForPingAsyncCompletion(%u)", nfds)

   if (nfds &&
       SHM_counter < nfds)
      {
      UTimeVal(1L).nanosleep();

      U_INTERNAL_DUMP("SHM_counter = %u addrmask = %B", SHM_counter, __FDS_BITS(addrmask)[0])

      // check if pending...

      if (SHM_counter < nfds) U_RETURN_POINTER(0, fd_set);
      }

   return pingAsyncCompletion();
}

void UPing::pingAsync(uint32_t nfd, UIPAddress* paddr, vPF unatexit, const char* device)
{
   U_TRACE(0, "UPing::pingAsync(%u,%p,%p,%S)", nfd, paddr, unatexit, device)

   if (proc->fork() &&
       proc->child())
      {
#  ifdef HAVE_NETPACKET_PACKET_H
      bool bresp = (device ? arping(*paddr, device) : ping(*paddr));
#  else
      bool bresp = (device ? false                  : ping(*paddr));
#  endif

      U_INTERNAL_DUMP("bresp = %b", bresp)

      if (bresp) FD_SET(nfd, addrmask);

      SHM_counter++;

      if (unatexit) u_unatexit(unatexit);

      U_EXIT(0);
      }
}

fd_set* UPing::ping(UVector<UIPAddress*>& vaddr, bool async, vPF unatexit, const char* device)
{
   U_TRACE(0, "UPing::ping(%p,%b,%p,%S)", &vaddr, async, unatexit, device)

   if (USocket::isClosed())
      {
#  ifdef HAVE_NETPACKET_PACKET_H
      if (device) initArpPing(device);
      else
#  endif
         initPing();
      }

   SHM_counter = 0;
   FD_ZERO(addrmask);

   U_INTERNAL_DUMP("SHM_counter = %d addrmask = %B", SHM_counter, __FDS_BITS(addrmask)[0])

   uint32_t n = vaddr.size();

   for (uint32_t i = 0; i < n; ++i) pingAsync(i, vaddr[i], unatexit, device);

   return checkForPingAsyncCompletion(async ? n : 0);
}

#ifdef HAVE_NETPACKET_PACKET_H

void UPing::initArpPing(const char* device)
{
   U_TRACE(0, "UPing::initArpPing(%S)", device)

   USocket::iSockDesc = USocket::socket(PF_PACKET, SOCK_DGRAM, 0);

   if (USocket::isClosed())
      {
      if (UServices::isSetuidRoot() == false) U_ERROR("Must run as root to create packet socket...", 0);

      U_ERROR("Sorry, could not create packet socket", 0);
      }

   struct ifreq ifr;

   (void) U_SYSCALL(memset, "%p,%C,%u", &ifr, '\0', sizeof(struct ifreq));

   (void) strncpy(ifr.ifr_name, device, IFNAMSIZ-1);

   if (U_SYSCALL(ioctl, "%d,%d,%p", USocket::iSockDesc, SIOCGIFINDEX, (char*)&ifr) < 0) U_ERROR("Unknown iface '%s'", device);

   me.l.sll_ifindex = ifr.ifr_ifindex;

   if (U_SYSCALL(ioctl, "%d,%d,%p", USocket::iSockDesc, SIOCGIFFLAGS, (char*)&ifr)) U_ERROR("ioctl(SIOCGIFFLAGS)", 0);

   if (!(ifr.ifr_flags &  IFF_UP))                   U_ERROR("Interface '%s' is down",        device);
   if (  ifr.ifr_flags & (IFF_NOARP | IFF_LOOPBACK)) U_ERROR("Interface '%s' is not ARPable", device);

   me.l.sll_family    = AF_PACKET;
   me.l.sll_protocol = htons(ETH_P_ARP);

   socklen_t alen = sizeof(struct sockaddr_ll);

   U_INTERNAL_DUMP("alen = %u", alen)

   if (U_SYSCALL(bind,        "%d,%p,%d", USocket::iSockDesc, &(me.s),  alen) == -1) U_ERROR_SYSCALL("bind");
   if (U_SYSCALL(getsockname, "%d,%p,%p", USocket::iSockDesc, &(me.s), &alen) == -1) U_ERROR_SYSCALL("getsockname");

   U_INTERNAL_DUMP("me.sll_halen = %u", me.l.sll_halen) // 6

   if (me.l.sll_halen == 0) U_ERROR("Interface '%s' is not ARPable (no ll address)", device);

   he = me;

   (void) U_SYSCALL(memset, "%p,%C,%u", he.l.sll_addr, -1, he.l.sll_halen);

   ah.pc = ah_buf;
   preq  = (unsigned char*)(ah.ph + 1);

                                            ah.ph->ar_hrd = htons(me.l.sll_hatype);
   if (ah.ph->ar_hrd == htons(ARPHRD_FDDI)) ah.ph->ar_hrd = htons(ARPHRD_ETHER);  // Format of hardware address
                                            ah.ph->ar_pro = htons(ETH_P_IP);      // Format of protocol address
                                            ah.ph->ar_hln = me.l.sll_halen;       // Length of hardware address
                                            ah.ph->ar_pln = 4;                    // Length of protocol address
                                            ah.ph->ar_op  = htons(ARPOP_REQUEST); // ARP opcode (command)

   U_INTERNAL_DUMP("ar_op = %u", ah.ph->ar_op)

   // ----------------------------------------------------------------
   // Ethernet looks like this : This bit is variable sized however...
   // ----------------------------------------------------------------

   // Sender hardware address

   (void) U_SYSCALL(memcpy, "%p,%p,%u", preq, me.l.sll_addr, me.l.sll_halen);

   preq += me.l.sll_halen;

   // Sender IP address

   (void) U_SYSCALL(memcpy, "%p,%p,%u", preq, USocket::cLocalAddress.get_in_addr(), 4);

   preq += 4;

   // Target hardware address

   (void) U_SYSCALL(memcpy, "%p,%p,%u", preq, he.l.sll_addr, ah.ph->ar_hln);

   preq += ah.ph->ar_hln;

   iPayloadLength = ((preq + 4) - ah_buf);

   U_INTERNAL_DUMP("iPayloadLength = %u", iPayloadLength)

   // Target IP address - TODO...
   // (void) U_SYSCALL(memcpy, "%p,%p,%u", preq, addr.get_in_addr(), 4);

   (void) setTimeoutSND(timeoutMS);
   (void) setTimeoutRCV(timeoutMS);
}

bool UPing::arping(UIPAddress& addr, const char* device)
{
   U_TRACE(0, "UPing::arping(%p,%S)", &addr, device)

   U_CHECK_MEMORY

   if (USocket::isClosed()) initArpPing(device);

   int ret;
   unsigned char* pres;
   struct in_addr* src_ip;
   struct in_addr* dst_ip;
   union uusockaddr_ll from;
   unsigned char packet[4096];
   socklen_t alen = sizeof(struct sockaddr_ll);
   struct in_addr* dst = (struct in_addr*)addr.get_in_addr();
   struct in_addr* src = (struct in_addr*)USocket::cLocalAddress.get_in_addr();

   // Target IP address

   (void) U_SYSCALL(memcpy, "%p,%p,%u", preq, addr.get_in_addr(), 4);

   ah.pc = packet;

   U_DUMP("local = %s addr = %s", inet_ntoa(*src), addr.getAddressString())

   for (int i = 0; i < 3; ++i)
      {
      // send ARP request

      ret = U_SYSCALL(sendto, "%d,%p,%d,%u,%p,%d", USocket::iSockDesc, CAST(ah_buf), iPayloadLength, 0, &(he.s), sizeof(struct sockaddr_ll));

      if (checkIO(ret, iPayloadLength) == false) U_RETURN(false);

      // wait for ARP response
loop:
      ret = U_SYSCALL(recvfrom, "%d,%p,%d,%u,%p,%p", USocket::iSockDesc, CAST(packet), sizeof(packet), 0, &(from.s), &alen);

      if (ret <= 0)
         {
         checkErrno(ret);

         if (isTimeout()) continue;

         U_RETURN(false);
         }

      // Filter out wild packets

      U_INTERNAL_DUMP("sll_pkttype = %u", from.l.sll_pkttype)

      if (from.l.sll_pkttype != PACKET_HOST      &&
          from.l.sll_pkttype != PACKET_BROADCAST &&
          from.l.sll_pkttype != PACKET_MULTICAST)
         {
         goto loop;
         }

      // Only these types are recognised

      U_INTERNAL_DUMP("ar_op = %u", ah.ph->ar_op)

      if (ah.ph->ar_op != htons(ARPOP_REQUEST) &&
          ah.ph->ar_op != htons(ARPOP_REPLY))
         {
         goto loop;
         }

      // ARPHRD check and this darned FDDI hack here :-(

      if (ah.ph->ar_hrd != htons(from.l.sll_hatype) &&
          (from.l.sll_hatype != ARPHRD_FDDI || ah.ph->ar_hrd != htons(ARPHRD_ETHER)))
         {
         goto loop;
         }

      // Protocol must be IP

      U_INTERNAL_DUMP("ar_pro = %u ar_pln = %u ar_hln = %u", ah.ph->ar_pro, ah.ph->ar_pln, ah.ph->ar_hln)

      if (ah.ph->ar_pro != htons(ETH_P_IP) ||
          ah.ph->ar_pln != 4               ||
          ah.ph->ar_hln != me.l.sll_halen  ||
          ret < iPayloadLength)
         {
         goto loop;
         }

      pres   = (unsigned char*)(ah.ph + 1) + ah.ph->ar_hln;
      src_ip = (struct in_addr*)pres;

      pres += 4;

      dst_ip = (struct in_addr*)(pres + ah.ph->ar_hln);

      U_INTERNAL_DUMP("%s %s from %s", (from.l.sll_pkttype == PACKET_HOST  ? "Unicast" : "Broadcast"),
                                       (ah.ph->ar_op == htons(ARPOP_REPLY) ? "reply"   : "request"), inet_ntoa(*src_ip))

      if (src_ip->s_addr != dst->s_addr ||
          dst_ip->s_addr != src->s_addr) goto loop;

      U_INTERNAL_DUMP("me.l.sll_addr = %#.*S pres = %#.*S", ah.ph->ar_hln, &(me.l.sll_addr), ah.ph->ar_hln, pres)

      if (U_SYSCALL(memcmp, "%p,%p,%u", pres, &(me.l.sll_addr), ah.ph->ar_hln)) goto loop;

      U_RETURN(true);
      }

   U_RETURN(false);
}

fd_set* UPing::arping(UPing** sockp, UVector<UIPAddress*>** vaddr, uint32_t n, bool async, vPF unatexit, UVector<UString>& vdev)
{
   U_TRACE(0, "UPing::arping(%p,%p,%u,%b,%p,%p)", sockp, vaddr, n, async, unatexit, &vdev)

   SHM_counter = 0;
   FD_ZERO(addrmask);

   uint32_t i, j, k, nfds = 0;

   for (i = 0; i < n; ++i)
      {
      k = vaddr[i]->size();

      for (j = 0; j < k; ++j) sockp[i]->pingAsync(nfds++, vaddr[i]->at(j), unatexit, vdev[i].data());
      }

   U_INTERNAL_DUMP("nfds = %u", nfds)

   return checkForPingAsyncCompletion(async ? nfds : 0);
}

#endif

fd_set* UPing::arpcache(UVector<UIPAddress*>** vaddr, uint32_t n)
{
   U_TRACE(0, "UPing::arpcache(%p,%u)", vaddr, n)

   SHM_counter = 0;
   FD_ZERO(addrmask);

   bool found;
   UString ip;
   uint32_t i, j, k;
   UIPAddress* paddr;
   UVector<UString> vec;

   USocketExt::getARPCache(vec);

   for (i = 0; i < n; ++i)
      {
      k = vaddr[i]->size();

      for (j = 0; j < k; ++j)
         {
         paddr = vaddr[i]->at(j);
         ip    = paddr->getAddressString();

         U_INTERNAL_DUMP("ip = %.*S", U_STRING_TO_TRACE(ip))

         found = vec.isContained(ip);

         U_INTERNAL_DUMP("found = %b", found)

         if (found) FD_SET(SHM_counter, addrmask);

         SHM_counter++;
         }
      }

   U_INTERNAL_DUMP("SHM_counter = %u addrmask = %B", SHM_counter, __FDS_BITS(addrmask)[0])

   U_RETURN_POINTER(addrmask, fd_set);
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UPing::dump(bool reset) const
{
   USocket::dump(false);

   *UObjectIO::os << '\n'
                  << "rep                           " << rep            << '\n'
                  << "timeoutMS                     " << timeoutMS      << '\n'
#              ifdef HAVE_NETPACKET_PACKET_H
                  << "preq                          " << (void*)preq    << '\n'
                  << "iPayloadLength                " << iPayloadLength << '\n'
#              endif
                  << "proc            (UProcess     " << (void*)proc    << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
