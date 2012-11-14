// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    ping.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_PING_H
#define ULIB_PING_H 1

#include <ulib/net/socket.h>

#ifndef __MINGW32__
#  include <sys/select.h>
#  ifdef HAVE_NETPACKET_PACKET_H
#     include <netpacket/packet.h>
#  endif
#endif

#define U_TIME_FOR_ARPING_ASYNC_COMPLETION 15

// PING (protocollo ICMP)
// -------------------------------------
// send icmp echo request and wait reply
// -------------------------------------
//
// ARPING (protocollo ARP)
// -------------------------------------
// send arp request and wait reply
// -------------------------------------

class UProcess;
class UNoCatPlugIn;

template <class T> class UVector;

class U_EXPORT UPing : public USocket {
public:

   typedef struct icmphdr {
      uint8_t type, code;
      uint16_t checksum;
   } icmphdr;

   typedef struct reqhdr {
      uint8_t type, code;
      uint16_t checksum, id, seq;
   } reqhdr;

   typedef struct rephdr {
      uint8_t type, code;
      uint16_t checksum, id, seq;
      int32_t ttl;
   } rephdr;

   // COSTRUTTORI

            UPing(int _timeoutMS = 3000, bool bSocketIsIPv6 = false);
   virtual ~UPing();

   /** This method is called to test whether a particular host is reachable across an IP network; it is also used to self test the network interface card
    * of the computer, or as a latency test. It works by sending ICMP echo request packets to the target host and listening for ICMP echo response replies.
    * Note that ICMP (and therefore ping) resides on the Network layer (level 3) of the OSI (Open Systems Interconnection) model. This is the same layer as
    * IP (Internet Protocol). Consequently, ping does not use a port for communication
    */

   void initPing();

   bool ping(UIPAddress& addr);

   bool ping(const UString& host)
      {
      U_TRACE(0, "UPing::ping(%.*S)", U_STRING_TO_TRACE(host))

      UIPAddress addr;

      bool result = (addr.setHostName(host, USocket::bIPv6Socket) && ping(addr));

      U_RETURN(result);
      }

#ifdef HAVE_NETPACKET_PACKET_H

   /* The arping command tests whether a given IP network address is in use on the local network, and can get additional information about the
    * device using that address. The arping command is similar in function to ping, but it operates using Address Resolution Protocol (ARP)
    * instead of Internet Control Message Protocol. Because it uses ARP, arping is only usable on the local network; in some cases the response
    * will be coming, not from the arpinged host, but rather from an intermediate system that engages in proxy ARP (such as a router).
    */

// #define U_ARP_WITH_BROADCAST

   void initArpPing(const char* device);

   bool arping(UIPAddress& addr, const char* device = "eth0");

   bool arping(const UString& host, const char* device = "eth0")
      {
      U_TRACE(0, "UPing::arping(%.*S,%S)", U_STRING_TO_TRACE(host), device)

      UIPAddress addr;

      bool result = (addr.setHostName(host, USocket::bIPv6Socket) && arping(addr, device));

      U_RETURN(result);
      }

   static fd_set* arping(UPing** sockp, UVector<UIPAddress*>** vaddr, uint32_t n, bool async, vPF unatexit, UVector<UString>& vdev);
#endif

   static fd_set* arpcache(UVector<UIPAddress*>** vaddr, uint32_t n);

   // parallel PING/ARPING

   static fd_set* pingAsyncCompletion();
   static fd_set* checkForPingAsyncCompletion(uint32_t nfds);
          fd_set* ping(UVector<UIPAddress*>& vaddr, bool async, vPF unatexit, const char* device);

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   reqhdr  req;
   rephdr* rep;
   int timeoutMS;

#ifdef HAVE_NETPACKET_PACKET_H
   typedef struct arpmsg {
      // Ethernet header
      uint8_t  h_dest[6];     // 00 destination ether addr
      uint8_t  h_source[6];   // 06 source ether addr
      uint16_t h_proto;       // 0c packet type ID field
      // ARP packet
      uint16_t htype;         // 0e hardware type (must be ARPHRD_ETHER)
      uint16_t ptype;         // 10 protocol type (must be ETH_P_IP)
      uint8_t  hlen;          // 12 hardware address length (must be 6)
      uint8_t  plen;          // 13 protocol address length (must be 4)
      uint16_t operation;     // 14 ARP opcode
      uint8_t  sHaddr[6];     // 16 sender's hardware address
      uint8_t  sInaddr[4];    // 1c sender's IP address
      uint8_t  tHaddr[6];     // 20 target's hardware address
      uint8_t  tInaddr[4];    // 26 target's IP address
      uint8_t  pad[18];       // 2a pad for min. ethernet payload (60 bytes)
   } arpmsg;

   arpmsg arp;

   union uusockaddr_ll {
      struct sockaddr    s;
      struct sockaddr_ll l;
   };

#  ifndef U_ARP_WITH_BROADCAST
   union uusockaddr_ll he;
#  endif

   int recvArpPing();
#endif

   static bool bmmap;
   static UProcess* proc;
   static fd_set* addrmask;

   void pingAsync(uint32_t nfd, UIPAddress* paddr, vPF unatexit, const char* device);

private:

   // Checksum (16 bits), calculated with the ICMP part of the packet (the IP header is not used)

   static inline void cksum(void* hdr, int len) U_NO_EXPORT;

   UPing(const UPing&) : USocket(false) {}
   UPing& operator=(const UPing&)       { return *this; }

   friend class UNoCatPlugIn;
};

#endif
