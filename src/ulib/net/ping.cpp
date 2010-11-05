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

inline void UPing::cksum(void* hdr, int len)
{
   U_TRACE(0, "UPing::cksum(%p,%d)", hdr, len)

   int i = 0, b1 = 0, b2 = 0;
   uint8_t* cb = (uint8_t*)hdr;

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

   U_INTERNAL_DUMP("b1 = %ld b2 = %ld", b1, b2)
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
      if (UServices::isSetuidRoot() == false) U_ERROR("Must run as root to create raw socket...");

      U_ERROR("Sorry, could not create raw socket");
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
   uint16_t seq = 0;
   UIPAddress cResponseIP;
   unsigned char buf[4096];
   int iphdrlen, ret, iSourcePortNumber;

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

   USocket::iSockDesc = USocket::socket(PF_PACKET, SOCK_PACKET, htons(ETH_P_ARP));

   if (USocket::isClosed())
      {
      if (UServices::isSetuidRoot() == false) U_ERROR("Must run as root to create packet socket...");

      U_ERROR("Sorry, could not create packet socket");
      }

   uint32_t value = 1;

   if (USocket::setSockOpt(SOL_SOCKET, SO_BROADCAST, (const void*)&value, sizeof(uint32_t)) == false)
      {
      U_ERROR("can't enable bcast on packet socket");
      }

   struct ifreq ifr;

   (void) U_SYSCALL(memset, "%p,%C,%u", &ifr, '\0', sizeof(struct ifreq));

   (void) strncpy(ifr.ifr_name, device, IFNAMSIZ-1);

   if (U_SYSCALL(ioctl, "%d,%d,%p", USocket::iSockDesc, SIOCGIFINDEX, (char*)&ifr) == -1) U_ERROR("Unknown iface for interface %S", device);
   if (U_SYSCALL(ioctl, "%d,%d,%p", USocket::iSockDesc, SIOCGIFFLAGS, (char*)&ifr) == -1) U_ERROR("ioctl(SIOCGIFFLAGS) failed for interface %S", device);

   if (!(ifr.ifr_flags &  IFF_UP))                   U_ERROR("Interface %S is down",        device);
   if (  ifr.ifr_flags & (IFF_NOARP | IFF_LOOPBACK)) U_ERROR("Interface %S is not ARPable", device);

   (void) U_SYSCALL(memset, "%p,%C,%u", &arp, '\0', sizeof(arp));

   arp.h_proto    = htons(ETH_P_ARP);     /* protocol type (Ethernet) */
   arp.htype      = htons(ARPHRD_ETHER);  /* hardware type */
   arp.ptype      = htons(ETH_P_IP);      /* protocol type (ARP message) */
   arp.hlen       = 6;                    /* hardware address length */
   arp.plen       = 4;                    /* protocol address length */
   arp.operation  = htons(ARPOP_REQUEST); /* ARP op code */

   if (U_SYSCALL(ioctl, "%d,%d,%p", USocket::iSockDesc, SIOCGIFADDR,   (char*)&ifr) == -1) U_ERROR("ioctl(SIOCGIFADDR) failed for interface %S", device);

   (void) U_SYSCALL(memcpy, "%p,%p,%u", arp.sInaddr, ifr.ifr_addr.sa_data + sizeof(short), 4); /* source IP address */

   if (U_SYSCALL(ioctl, "%d,%d,%p", USocket::iSockDesc, SIOCGIFHWADDR, (char*)&ifr) == -1) U_ERROR("ioctl(SIOCGIFHWADDR) failed for interface %S", device);

   (void) U_SYSCALL(memset, "%p,%C,%u", arp.h_dest,                     0xff,               6); /* MAC DA */
   (void) U_SYSCALL(memcpy, "%p,%p,%u", arp.h_source, ifr.ifr_hwaddr.sa_data,               6); /* MAC SA */
   (void) U_SYSCALL(memcpy, "%p,%p,%u", arp.sHaddr,   ifr.ifr_hwaddr.sa_data,               6); /* source hardware address */

   U_INTERNAL_DUMP("SOURCE = %s (%02x:%02x:%02x:%02x:%02x:%02x)",
                      u_inet_nstoa(arp.sInaddr),
                      arp.sHaddr[0] & 0xFF,
                      arp.sHaddr[1] & 0xFF,
                      arp.sHaddr[2] & 0xFF,
                      arp.sHaddr[3] & 0xFF,
                      arp.sHaddr[4] & 0xFF,
                      arp.sHaddr[5] & 0xFF)

   (void) setTimeoutSND(timeoutMS);
   (void) setTimeoutRCV(timeoutMS);

   // -----------------------------------------------------------------------------------------------------------------------
   // Target address - TODO...
   // -----------------------------------------------------------------------------------------------------------------------
   //                                      arp.tHaddr is zero-filled                            /* target hardware address */
   // (void) U_SYSCALL(memcpy, "%p,%p,%u", arp.tInaddr, addr.get_in_addr(), 4);                 /* target IP address */
   // -----------------------------------------------------------------------------------------------------------------------
}

bool UPing::arping(UIPAddress& addr, const char* device)
{
   U_TRACE(0, "UPing::arping(%p,%S)", &addr, device)

   U_CHECK_MEMORY

   if (USocket::isClosed()) initArpPing(device);

   // -----------------------------------------------------------------------------------------------------------------------
   // Target address
   // -----------------------------------------------------------------------------------------------------------------------
   //                                   arp.tHaddr is zero-filled                            /* target hardware address */
   (void) U_SYSCALL(memcpy, "%p,%p,%u", arp.tInaddr, addr.get_in_addr(), 4);                 /* target IP address */
   // -----------------------------------------------------------------------------------------------------------------------

   U_DUMP("ARP request from %s (%02x:%02x:%02x:%02x:%02x:%02x) to %s (%02x:%02x:%02x:%02x:%02x:%02x)",
            u_inet_nstoa(arp.sInaddr),
            arp.sHaddr[0] & 0xFF, arp.sHaddr[1] & 0xFF, arp.sHaddr[2] & 0xFF,
            arp.sHaddr[3] & 0xFF, arp.sHaddr[4] & 0xFF, arp.sHaddr[5] & 0xFF,
            addr.getAddressString(),
            arp.tHaddr[0] & 0xFF, arp.tHaddr[1] & 0xFF, arp.tHaddr[2] & 0xFF,
            arp.tHaddr[3] & 0xFF, arp.tHaddr[4] & 0xFF, arp.tHaddr[5] & 0xFF)

   int ret;
   arpmsg reply;
   struct sockaddr saddr;

   (void) U_SYSCALL(memset, "%p,%C,%u", &saddr, '\0', sizeof(struct sockaddr));

   (void) strncpy(saddr.sa_data, device, sizeof(saddr.sa_data));

   for (int i = 0; i < 3; ++i)
      {
      // send ARP request

      ret = U_SYSCALL(sendto, "%d,%p,%d,%u,%p,%d", USocket::iSockDesc, &arp, sizeof(arp), 0, &saddr, sizeof(struct sockaddr));

      if (checkIO(ret, sizeof(arp)) == false) U_RETURN(false);

      // wait for ARP reply
loop:
      ret = U_SYSCALL(recv, "%d,%p,%d,%u,%p,%p", USocket::iSockDesc, &reply, sizeof(reply), 0);

      if (ret <= 0)
         {
         checkErrno(ret);

         if (isTimeout()) continue;

         U_RETURN(false);
         }

      U_INTERNAL_DUMP("h_proto = %u htype = %u ptype = %u hlen = %u plen = %u operation = %u",
                        reply.h_proto, reply.htype, reply.ptype, reply.hlen, reply.plen, reply.operation)

      if (ret < 42 && // ARP_MSG_SIZE
          reply.operation != htons(ARPOP_REPLY))
         {
         goto loop;
         }

#  ifdef DEBUG
      char str_from[32], str_to[32];

      str_from[0] = str_to[0] = '\0';

      (void) strcpy(str_from, u_inet_nstoa(reply.sInaddr));
      (void) strcpy(str_to,   u_inet_nstoa(reply.tInaddr));

      U_INTERNAL_DUMP("ARP reply from %s (%02x:%02x:%02x:%02x:%02x:%02x) to %s (%02x:%02x:%02x:%02x:%02x:%02x)",
                        str_from,
                        reply.sHaddr[0] & 0xFF, reply.sHaddr[1] & 0xFF, reply.sHaddr[2] & 0xFF,
                        reply.sHaddr[3] & 0xFF, reply.sHaddr[4] & 0xFF, reply.sHaddr[5] & 0xFF,
                        str_to,
                        reply.tHaddr[0] & 0xFF, reply.tHaddr[1] & 0xFF, reply.tHaddr[2] & 0xFF,
                        reply.tHaddr[3] & 0xFF, reply.tHaddr[4] & 0xFF, reply.tHaddr[5] & 0xFF)
#  endif

      // NB: don't check tHaddr: Linux doesn't return proper target's hardware address (fixed in 2.6.24?)

      if (memcmp(reply.tInaddr, arp.sInaddr, 4) ||
          memcmp(reply.tHaddr,  arp.sHaddr,  6))
         {
         goto loop;
         }

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
                  << "proc            (UProcess     " << (void*)proc    << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
