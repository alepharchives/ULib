// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    socket_ext.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/file.h>
#include <ulib/notifier.h>
#include <ulib/container/vector.h>
#include <ulib/utility/socket_ext.h>

#ifdef USE_LIBSSL
#  include <ulib/ssl/net/sslsocket.h>
#endif

#ifdef __MINGW32__
#  include <ws2tcpip.h>
#elif defined(HAVE_NETPACKET_PACKET_H) && !defined(U_ALL_CPP)
#  include <net/if.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
#  include <sys/ioctl.h>
#endif

vPFi USocketExt::byte_read_hook; // it allows the generation of a progress meter during upload...

// Socket I/O

// read while not received almost count data
//
// timeoutMS  specified the timeout value, in milliseconds. A negative value indicates no timeout, i.e. an infinite wait
// time_limit specified the maximum execution time, in seconds. If set to zero, no time limit is imposed

bool USocketExt::read(USocket* s, UString& buffer, int count, int timeoutMS, uint32_t time_limit)
{
   U_TRACE(0, "USocketExt::read(%p,%.*S,%d,%d,%u)", s, U_STRING_TO_TRACE(buffer), count, timeoutMS, time_limit)

   U_INTERNAL_ASSERT_POINTER(s)
   U_INTERNAL_ASSERT_DIFFERS(count,0)
   U_INTERNAL_ASSERT(s->isConnected())

   ssize_t value;
   long timeout = 0;
   int byte_read = 0;
   bool result = true;
   uint32_t start  = buffer.size(), // NB: buffer read can start with prev data...
            ncount = buffer.space(),
            chunk  = U_max(count,(int)U_CAPACITY);

   if (ncount < chunk &&
       buffer.reserve(chunk))
      {
      ncount = buffer.rep->space();
      }

   char* ptr = buffer.c_pointer(start);

read:
   value = s->recv(ptr + byte_read, ncount, timeoutMS);

   if (value <= 0L)
      {
      if (value == 0L) s->close(); // EOF
      else
         {
         s->checkErrno();

         // NB: maybe we have failed to read more bytes...

         if (byte_read &&
             s->isTimeout())
            {
            U_INTERNAL_ASSERT_EQUALS(timeoutMS, 500)

            s->iState = USocket::CONNECT;

            U_RETURN(true);
            }
         }

      result = false;

      goto done;
      }

   byte_read += value;

   U_INTERNAL_DUMP("byte_read = %d", byte_read)

   if (byte_read < count)
      {
      U_INTERNAL_ASSERT_DIFFERS(count,U_SINGLE_READ)

      if (time_limit &&
          s->checkTime(time_limit, timeout) == false) // NB: may be is attacked by a "slow loris"... http://lwn.net/Articles/337853/
         {
         result = false;

         goto done;
         }

      if (byte_read_hook) byte_read_hook(byte_read);

      ncount -= byte_read;

      goto read;
      }

   if (value == (ssize_t)ncount)
      {
      // NB: may be there are available more bytes to read...

      buffer.size_adjust_force(start + byte_read); // NB: we force because string can be referenced...

      if (buffer.reserve(start + byte_read + ncount)) ptr = buffer.c_pointer(start);

#  ifdef USE_LIBSSL
      if (s->isSSL())
         {
         /* 
          * When packets in SSL arrive at a destination, they are pulled off the socket in chunks of sizes controlled by the encryption protocol being
          * used, decrypted, and placed in SSL-internal buffers. The buffer content is then transferred to the application program through SSL_read().
          * If you've read only part of the decrypted data, there will still be pending input data on the SSL connection, but it won't show up on the
          * underlying file descriptor via select(). Your code needs to call SSL_pending() explicitly to see if there is any pending data to be read.
          */

         uint32_t available = ((USSLSocket*)s)->pending();

         if (available)
            {
            byte_read += s->recv(ptr + byte_read, available);

            goto done;
            }
         }
#  endif

      timeoutMS = 500; // wait max for half second...

      ncount = buffer.rep->space();

      goto read;
      }

done:
   buffer.size_adjust_force(start + byte_read); // NB: we force because string can be referenced...

   U_RETURN(result);
}

// param timeoutMS specified the timeout value, in milliseconds.
// A negative value indicates no timeout, i.e. an infinite wait

// read while not received token, return position of token in buffer

uint32_t
USocketExt::readWhileNotToken(USocket* s, UString& buffer, const char* token, uint32_t token_len, uint32_t max_read, int timeoutMS, uint32_t time_limit)
{
   U_TRACE(0, "USocketExt::readWhileNotToken(%p,%.*S,%.*S,%u,%u,%u)", s, U_STRING_TO_TRACE(buffer),
                                                                      token_len, token, token_len, max_read, timeoutMS, time_limit)

   U_INTERNAL_ASSERT(s->isConnected())

   long timeout = 0;
   uint32_t count = 0, pos_token, start = buffer.size();

   while (USocketExt::read(s, buffer, U_SINGLE_READ, timeoutMS, time_limit))
      {
      pos_token = buffer.find(token, start, token_len);

      if (pos_token != U_NOT_FOUND) U_RETURN(pos_token);

      // NB: may be is attacked by a "slow loris"... http://lwn.net/Articles/337853/

      U_INTERNAL_DUMP("slow loris count = %u", count)

      if (count++ > max_read ||
          (time_limit &&
           s->checkTime(time_limit, timeout) == false))
         {
         U_RETURN(U_NOT_FOUND);
         }

      U_ASSERT_MAJOR(buffer.size(), token_len)

      start = buffer.size() - token_len;
      }

   U_RETURN(U_NOT_FOUND);
}

// write while sending data

int USocketExt::write(USocket* s, const char* ptr, uint32_t ncount, int timeoutMS)
{
   U_TRACE(0, "USocketExt::write(%p,%.*S,%u,%d)", s, ncount, ptr, ncount, timeoutMS)

   U_INTERNAL_ASSERT(s->isOpen())

   ssize_t value;
   int iBytesWrite = 0;

   while (true)
      {
      value = s->send(ptr, ncount, timeoutMS);

      if (s->checkIO(value) == false) break;

      iBytesWrite += value;
      ncount      -= value;

      U_INTERNAL_DUMP("ncount = %u", ncount)

      if (ncount == 0) break;

      ptr += value;
      }

   U_RETURN(iBytesWrite);
}

int USocketExt::writev(USocket* s, struct iovec* _iov, int iovcnt, uint32_t ncount, int timeoutMS)
{
   U_TRACE(0, "USocketExt::writev(%p,%p,%d,%u,%d)", s, _iov, iovcnt, ncount, timeoutMS)

   U_INTERNAL_ASSERT(s->isOpen())
   U_INTERNAL_ASSERT_MAJOR(ncount, 0)

   ssize_t value;
   int idx = 0, iBytesWrite = 0;
#ifdef DEBUG
   int i;
   uint32_t sum;
#endif

loop:
   U_INTERNAL_DUMP("ncount = %u _iov[%d].iov_len = %d", ncount, idx, _iov[idx].iov_len)

#ifdef DEBUG
   for (i = sum = 0; i < iovcnt; ++i) sum += _iov[i].iov_len;

   U_INTERNAL_DUMP("sum = %u", sum)

   U_INTERNAL_ASSERT_EQUALS(sum, ncount)
#endif

   value = s->writev(_iov, iovcnt, timeoutMS);

   if (s->checkIO(value) == false) U_RETURN(iBytesWrite);

   iBytesWrite += value;
   ncount      -= value;

   U_INTERNAL_DUMP("ncount = %u", ncount)

   if (ncount == 0) U_RETURN(iBytesWrite);

   U_INTERNAL_ASSERT_MAJOR(_iov[idx].iov_len, 0)

   if ((size_t)value >= _iov[idx].iov_len)
      {
      value -= _iov[idx].iov_len;
               _iov[idx].iov_len = 0;

      ++idx;

      U_INTERNAL_ASSERT_MINOR(idx, iovcnt)
      }

   _iov[idx].iov_len -= value;
   _iov[idx].iov_base = (char*)_iov[idx].iov_base + value;

   int res = UNotifier::waitForWrite(s->iSockDesc, timeoutMS);

   if (res ==  1) goto loop;
   if (res == -1)
      {
      s->iState = USocket::BROKEN;

      s->_closesocket();
      }

   U_RETURN(iBytesWrite);
}

// Send a command to a server and wait for a response (single line)

int USocketExt::vsyncCommand(USocket* s, char* buffer, uint32_t buffer_size, const char* format, va_list argp)
{
   U_TRACE(0, "USocketExt::vsyncCommand(%p,%p,%u,%S)", s, buffer, buffer_size, format)

   U_INTERNAL_ASSERT(s->isOpen())

   uint32_t buffer_len = u__vsnprintf(buffer, buffer_size-2, format, argp);

   buffer[buffer_len++] = '\r';
   buffer[buffer_len++] = '\n';

   int n        = s->send(buffer, buffer_len),
       response = (s->checkIO(n) ? readLineReply(s, buffer, buffer_size) : (int)USocket::BROKEN);

   U_RETURN(response);
}

// Send a command to a server and wait for a response (multi line)

int USocketExt::vsyncCommandML(USocket* s, char* buffer, uint32_t buffer_size, const char* format, va_list argp)
{
   U_TRACE(0, "USocketExt::vsyncCommandML(%p,%p,%u,%S)", s, buffer, buffer_size, format)

   U_INTERNAL_ASSERT(s->isOpen())

   uint32_t buffer_len = u__vsnprintf(buffer, buffer_size-2, format, argp);

   buffer[buffer_len++] = '\r';
   buffer[buffer_len++] = '\n';

   int n        = s->send(buffer, buffer_len),
       response = (s->checkIO(n) ? readMultilineReply(s, buffer, buffer_size) : (int)USocket::BROKEN);

   U_RETURN(response);
}

// Send a command to a server and wait for a response (check for token line)

int USocketExt::vsyncCommandToken(USocket* s, UString& buffer, const char* format, va_list argp)
{
   U_TRACE(1, "USocketExt::vsyncCommandToken(%p,%.*S,%S)", s, U_STRING_TO_TRACE(buffer), format)

   U_INTERNAL_ASSERT(s->isOpen())
   U_INTERNAL_ASSERT_EQUALS((bool)buffer, false)

   static uint32_t cmd_count;

   char token[32];
   uint32_t token_len = u__snprintf(token, sizeof(token), "U%04u ", cmd_count++);

   U_INTERNAL_DUMP("token = %.*S", token_len, token)

   char* p = buffer.data();

   U__MEMCPY(p, token, token_len);

   uint32_t buffer_len = token_len + u__vsnprintf(p+token_len, buffer.capacity(), format, argp);

   p[buffer_len++] = '\r';
   p[buffer_len++] = '\n';

   int n = s->send(p, buffer_len);

   if (s->checkIO(n))
      {
      uint32_t pos_token = USocketExt::readWhileNotToken(s, buffer, token, token_len);

      if (pos_token != U_NOT_FOUND)
         {
                          U_ASSERT(buffer.c_char(buffer.size()-1) == '\n')
#     if defined(DEBUG) || defined(U_TEST)
         if (pos_token) { U_ASSERT(buffer.c_char(pos_token-1)     == '\n') }
#     endif

         U_RETURN(pos_token + token_len);
         }

      U_RETURN(U_NOT_FOUND);
      }

   U_RETURN(USocket::BROKEN);
}

inline bool USocketExt::parseCommandResponse(char* buffer, int r, int response)
{
   U_TRACE(0, "USocketExt::parseCommandResponse(%p,%d,%d)", buffer, r, response)

   /*
   Thus the format for multi-line replies is that the first line will begin with the exact required reply code,
   followed immediately by a Hyphen, "-" (also known as Minus), followed by text. The last line will begin with
   the same code, followed immediately by Space <SP>, optionally some text, and the Telnet end-of-line code.
   For example:
      123-First line
        Second line
       234 A line beginning with numbers
      123 The last line
   The user-process then simply needs to search for the second occurrence of the same reply code, followed by
   <SP> (Space), at the beginning of a line, and ignore all intermediary lines. If an intermediary line begins
   with a 3-digit number, the Server must pad the front to avoid confusion.
   */

   int complete = 2;

   if (buffer[3] == '-')
      {
      complete = 0;

      for (int i = 0; i < r; ++i)
         {
         if (buffer[i] == '\n')
            {
            if (complete == 1)
               {
               complete = 2;

               break;
               }

            U_INTERNAL_DUMP("buffer = %S", buffer+i+1)

            if (buffer[i+4] == ' ')
               {
               int j = -1;

               (void) sscanf(buffer+i+1, "%3i", &j);

               U_INTERNAL_DUMP("j = %d response = %d", j, response)

               if (j == response) complete = 1;
               }
            }
         }
      }

   U_INTERNAL_DUMP("complete = %d", complete)

   U_RETURN(complete != 2);
}

int USocketExt::readLineReply(USocket* s, char* buffer, uint32_t buffer_size) // response from server (single line)
{
   U_TRACE(0, "USocketExt::readLineReply(%p,%p,%u)", s, buffer, buffer_size)

   U_INTERNAL_ASSERT(s->isConnected())

   int i, r = 0, count;

   do {
      count = buffer_size - r;

      i = s->recv(buffer + r, count);

      if (s->checkIO(i) == false) U_RETURN(USocket::BROKEN);

      r += i;
      }
   while (buffer[r-1] != '\n');

   buffer[r] = '\0';

   U_RETURN(r);
}

int USocketExt::readMultilineReply(USocket* s, char* buffer, uint32_t buffer_size) // response from server (multi line)
{
   U_TRACE(0, "USocketExt::readMultilineReply(%p,%p,%u)", s, buffer, buffer_size)

   U_INTERNAL_ASSERT(s->isConnected())

   int r = 0, response;

   do {
      r = readLineReply(s, buffer + r, buffer_size - r);

      if (r == USocket::BROKEN) U_RETURN(USocket::BROKEN);

      response = atoi(buffer);
      }
   while (parseCommandResponse(buffer, r, response));

   U_RETURN(response);
}

// SERVICES

UString USocketExt::getNetworkDevice(const char* exclude)
{
   U_TRACE(1, "USocketExt::getNetworkDevice(%S)", exclude)

   UString result(100U);

#if !defined(__MINGW32__) && defined(HAVE_SYS_IOCTL_H)
   FILE* route = (FILE*) U_SYSCALL(fopen, "%S,%S", "/proc/net/route", "r");

   // Skip first line

   if (U_SYSCALL(fscanf, "%p,%S", route, "%*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s") != EOF)
      {
      bool found;
      char dev[7], dest[9];

      while (U_SYSCALL(fscanf, "%p,%S", route, "%6s %8s %*s %*s %*s %*s %*s %*s %*s %*s %*s\n", dev, dest) != EOF)
         {
         found = (exclude ? (strncmp(dev, exclude, 6) != 0)   // not the whatever it is
                          : (strcmp(dest, "00000000") == 0)); // default route

         if (found)
            {
            (void) result.assign(dev);

            break;
            }
         }
      }

   (void) U_SYSCALL(fclose, "%p", route);
#endif

   U_RETURN_STRING(result);
}

UString USocketExt::getNetworkAddress(int fd, const char* device)
{
   U_TRACE(1, "USocketExt::getNetworkAddress(%d,%S)", fd, device)

   U_INTERNAL_ASSERT(fd != -1)
   U_INTERNAL_ASSERT_POINTER(device)

   UString result(100U);
#if !defined(__MINGW32__) && defined(HAVE_SYS_IOCTL_H)
   struct ifreq ifaddr, ifnetmask;

   (void) u__strncpy(   ifaddr.ifr_name, device, IFNAMSIZ-1);
   (void) u__strncpy(ifnetmask.ifr_name, device, IFNAMSIZ-1);

   (void) U_SYSCALL(ioctl, "%d,%d,%p", fd,    SIOCGIFADDR, &ifaddr);
   (void) U_SYSCALL(ioctl, "%d,%d,%p", fd, SIOCGIFNETMASK, &ifnetmask);

   char* addr = ifaddr.ifr_addr.sa_data;
   char* mask = ifnetmask.ifr_netmask.sa_data;

   result.snprintf("%d.%d.%d.%d/%d.%d.%d.%d",
                   addr[0] & mask[0] & 0xFF,
                   addr[1] & mask[1] & 0xFF,
                   addr[2] & mask[2] & 0xFF,
                   addr[3] & mask[3] & 0xFF,
                             mask[0] & 0xFF,
                             mask[1] & 0xFF,
                             mask[2] & 0xFF,
                             mask[3] & 0xFF);
#endif

   U_RETURN_STRING(result);
}

void USocketExt::getARPCache(UVector<UString>& vec)
{
   U_TRACE(1, "USocketExt::getARPCache(%p)", &vec)

#if !defined(__MINGW32__) && defined(HAVE_SYS_IOCTL_H)
   FILE* arp = (FILE*) U_SYSCALL(fopen, "%S,%S", "/proc/net/arp", "r");

   // ------------------------------------------------------------------------------
   // Skip the first line
   // ------------------------------------------------------------------------------
   // IP address       HW type     Flags       HW address            Mask     Device
   // 192.168.253.1    0x1         0x2         00:14:a5:6e:9c:cb     *        ath0
   // 10.30.1.131      0x1         0x2         00:16:ec:fb:46:da     *        eth0
   // ------------------------------------------------------------------------------

   if (U_SYSCALL(fscanf, "%p,%S", arp, "%*s %*s %*s %*s %*s %*s %*s %*s %*s") != EOF)
      {
      char _ip[16];

      while (U_SYSCALL(fscanf, "%p,%S", arp, "%15s %*s %*s %*s %*s %*s\n", _ip) != EOF)
         {
         UString item((void*)_ip);

         vec.push(item);
         }
      }

   (void) U_SYSCALL(fclose, "%p", arp);
#endif
}

UString USocketExt::getNetworkInterfaceName(const char* ip)
{
   U_TRACE(1, "USocketExt::getNetworkInterfaceName(%S)", ip)

   U_INTERNAL_ASSERT(u_isIPv4Addr(ip,u__strlen(ip)))

   UString result(100U);

#if !defined(__MINGW32__) && defined(HAVE_SYS_IOCTL_H)
   FILE* arp = (FILE*) U_SYSCALL(fopen, "%S,%S", "/proc/net/arp", "r");

   // ------------------------------------------------------------------------------
   // Skip the first line
   // ------------------------------------------------------------------------------
   // IP address       HW type     Flags       HW address            Mask     Device
   // 192.168.253.1    0x1         0x2         00:14:a5:6e:9c:cb     *        ath0
   // 10.30.1.131      0x1         0x2         00:16:ec:fb:46:da     *        eth0
   // ------------------------------------------------------------------------------

   if (U_SYSCALL(fscanf, "%p,%S", arp, "%*s %*s %*s %*s %*s %*s %*s %*s %*s") != EOF)
      {
      char _ip[16], dev[16];

      while (U_SYSCALL(fscanf, "%p,%S", arp, "%15s %*s %*s %*s %*s %15s\n", _ip, dev) != EOF)
         {
         if (strcmp(ip, _ip) == 0)
            {
            (void) result.assign(dev);

            break;
            }
         }
      }

   (void) U_SYSCALL(fclose, "%p", arp);
#endif

   U_RETURN_STRING(result);
}

UString USocketExt::getMacAddress(int fd, const char* device_or_ip)
{
   U_TRACE(1, "USocketExt::getMacAddress(%d,%S)", fd, device_or_ip)

   U_INTERNAL_ASSERT_POINTER(device_or_ip)

   UString result(100U);

#if !defined(__MINGW32__) && defined(HAVE_SYS_IOCTL_H)
   if (u_isIPv4Addr(device_or_ip, u__strlen(device_or_ip)))
      {
      FILE* arp = (FILE*) U_SYSCALL(fopen, "%S,%S", "/proc/net/arp", "r");

      // ------------------------------------------------------------------------------
      // Skip the first line
      // ------------------------------------------------------------------------------
      // IP address       HW type     Flags       HW address            Mask     Device
      // 192.168.253.1    0x1         0x2         00:14:a5:6e:9c:cb     *        ath0
      // 10.30.1.131      0x1         0x2         00:16:ec:fb:46:da     *        eth0
      // ------------------------------------------------------------------------------

      if (U_SYSCALL(fscanf, "%p,%S", arp, "%*s %*s %*s %*s %*s %*s %*s %*s %*s") != EOF)
         {
         bool found;
         char ip[16], hw[18];

         while (U_SYSCALL(fscanf, "%p,%S", arp, "%15s %*s %*s %17s %*s %*s\n", ip, hw) != EOF)
            {
            found = (strncmp(device_or_ip, ip, sizeof(ip)) == 0);

            if (found)
               {
               (void) result.assign(hw);

               break;
               }
            }
         }

      (void) U_SYSCALL(fclose, "%p", arp);
      }
   else
      {
      U_INTERNAL_ASSERT(fd != -1)

      struct ifreq ifr;

      (void) u__strncpy(ifr.ifr_name, device_or_ip, IFNAMSIZ-1);

      (void) U_SYSCALL(ioctl, "%d,%d,%p", fd, SIOCGIFHWADDR, &ifr);

      char* hwaddr = ifr.ifr_hwaddr.sa_data;

      result.snprintf("%02x:%02x:%02x:%02x:%02x:%02x",
                      hwaddr[0] & 0xFF,
                      hwaddr[1] & 0xFF,
                      hwaddr[2] & 0xFF,
                      hwaddr[3] & 0xFF,
                      hwaddr[4] & 0xFF,
                      hwaddr[5] & 0xFF);
      }
#endif

   U_RETURN_STRING(result);
}

UString USocketExt::getIPAddress(int fd, const char* device)
{
   U_TRACE(1, "USocketExt::getIPAddress(%d,%S)", fd, device)

   U_INTERNAL_ASSERT(fd != -1)
   U_INTERNAL_ASSERT_POINTER(device)

   UString result(100U);

#if !defined(__MINGW32__) && defined(HAVE_SYS_IOCTL_H)
   struct ifreq ifr;

   (void) u__strncpy(ifr.ifr_name, device, IFNAMSIZ-1);

   /* Get the IP address of the interface */

   (void) U_SYSCALL(ioctl, "%d,%d,%p", fd, SIOCGIFADDR, &ifr);

   uusockaddr addr;

   U__MEMCPY(&addr, &ifr.ifr_addr, sizeof(struct sockaddr));

   U_INTERNAL_ASSERT_EQUALS(addr.psaIP4Addr.sin_family, AF_INET)

   (void) U_SYSCALL(inet_ntop, "%d,%p,%p,%u", AF_INET, &(addr.psaIP4Addr.sin_addr), result.data(), INET_ADDRSTRLEN);

   result.size_adjust();
#endif

   U_RETURN_STRING(result);
}

#if defined(LINUX) || defined(__LINUX__) || defined(__linux__)
#  include <linux/types.h>
#  include <linux/rtnetlink.h>
#endif

UString USocketExt::getGatewayAddress(const char* network, uint32_t network_len)
{
   U_TRACE(1, "USocketExt::getGatewayAddress(%.*S,%u)", network_len, network, network_len)

   UString result(100U);

   // Ex: ip route show to exact 192.168.1.0/24

#if defined(LINUX) || defined(__LINUX__) || defined(__linux__)
   static int sock = USocket::socket(AF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);

   if (sock != -1)
      {
      char msgBuf[4096];

      (void) memset(msgBuf, 0, 4096);

      /*
      struct nlmsghdr {
         __u32 nlmsg_len;    // Length of message including header
         __u16 nlmsg_type;   // Type of message content
         __u16 nlmsg_flags;  // Additional flags
         __u32 nlmsg_seq;    // Sequence number
         __u32 nlmsg_pid;    // PID of the sending process
      };
      */

      // point the header and the msg structure pointers into the buffer

      union uunlmsghdr {
         char*            p;
         struct nlmsghdr* h;
      };

      union uunlmsghdr nlMsg = { &msgBuf[0] };

      // Fill in the nlmsg header

      nlMsg.h->nlmsg_len   = NLMSG_LENGTH(sizeof(struct rtmsg)); // Length of message (28)
      nlMsg.h->nlmsg_type  = RTM_GETROUTE;                       // Get the routes from kernel routing table
      nlMsg.h->nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST;         // The message is a request for dump
      nlMsg.h->nlmsg_seq   = 0;                                  // Sequence of the message packet
      nlMsg.h->nlmsg_pid   = u_pid;                              // PID of process sending the request

      // Send the request

      if (U_SYSCALL(send, "%d,%p,%u,%u", sock, CAST(nlMsg.h), nlMsg.h->nlmsg_len, 0) == (ssize_t)nlMsg.h->nlmsg_len)
         {
         // Read the response

         int readLen;
         uint32_t msgLen = 0;
         char* bufPtr = msgBuf;
         union uunlmsghdr nlHdr;

         do {
            // Receive response from the kernel

            if ((readLen = U_SYSCALL(recv, "%d,%p,%u,%d", sock, CAST(bufPtr), 4096 - msgLen, 0)) < 0) break;

            nlHdr.p = bufPtr;

            // Check if the header is valid

            if ((NLMSG_OK(nlHdr.h, (uint32_t)readLen) == 0) || (nlHdr.h->nlmsg_type == NLMSG_ERROR)) break;

            // Check if it is the last message

            U_INTERNAL_DUMP("nlmsg_type = %u nlmsg_seq = %u nlmsg_pid = %u nlmsg_flags = %B",
                             nlHdr.h->nlmsg_type, nlHdr.h->nlmsg_seq, nlHdr.h->nlmsg_pid, nlHdr.h->nlmsg_flags)

            if (nlHdr.h->nlmsg_type == NLMSG_DONE) break;
            else
               {
               // Else move the pointer to buffer appropriately

               bufPtr += readLen;
               msgLen += readLen;
               }

            // Check if it is a multi part message

            if ((nlHdr.h->nlmsg_flags & NLM_F_MULTI) == 0) break;
            }
         while ((nlHdr.h->nlmsg_seq != 1) || (nlHdr.h->nlmsg_pid != (uint32_t)u_pid));

         U_INTERNAL_DUMP("msgLen = %u readLen = %u", msgLen, readLen)

         // Parse the response

         int rtLen;
         char* dst;
         char dstMask[32];
         struct rtmsg* rtMsg;
         struct rtattr* rtAttr;
         char ifName[IF_NAMESIZE];
         struct in_addr dstAddr, srcAddr, gateWay;

         for(; NLMSG_OK(nlMsg.h,msgLen); nlMsg.h = NLMSG_NEXT(nlMsg.h,msgLen))
            {
            rtMsg = (struct rtmsg*) NLMSG_DATA(nlMsg.h);

            U_INTERNAL_DUMP("rtMsg = %p msgLen = %u rtm_family = %u rtm_table = %u", rtMsg, msgLen, rtMsg->rtm_family, rtMsg->rtm_table)

            /*
            #define AF_INET   2 // IP protocol family
            #define AF_INET6 10 // IP version 6
            */

            if ((rtMsg->rtm_family != AF_INET)) continue; // If the route is not for AF_INET then continue

            /* Reserved table identifiers

            enum rt_class_t {
               RT_TABLE_UNSPEC=0,
               RT_TABLE_COMPAT=252,
               RT_TABLE_DEFAULT=253,
               RT_TABLE_MAIN=254,
               RT_TABLE_LOCAL=255,
               RT_TABLE_MAX=0xFFFFFFFF }; */

            if ((rtMsg->rtm_table != RT_TABLE_MAIN)) continue; // If the route does not belong to main routing table then continue

            ifName[0] = '\0';
            dstAddr.s_addr = srcAddr.s_addr = gateWay.s_addr = 0;

            // get the rtattr field

            rtAttr = (struct rtattr*) RTM_RTA(rtMsg);
            rtLen  = RTM_PAYLOAD(nlMsg.h);

            for (; RTA_OK(rtAttr,rtLen); rtAttr = RTA_NEXT(rtAttr,rtLen))
               {
               U_INTERNAL_DUMP("rtAttr = %p rtLen = %u rta_type = %u rta_len = %u", rtAttr, rtLen, rtAttr->rta_type, rtAttr->rta_len)

               /* Routing message attributes

               struct rtattr {
                  unsigned short rta_len;  // Length of option
                  unsigned short rta_type; //   Type of option
               // Data follows
               };

               enum rtattr_type_t {
                  RTA_UNSPEC,    // 0
                  RTA_DST,       // 1
                  RTA_SRC,       // 2
                  RTA_IIF,       // 3
                  RTA_OIF,       // 4
                  RTA_GATEWAY,   // 5
                  RTA_PRIORITY,  // 6
                  RTA_PREFSRC,   // 7
                  RTA_METRICS,   // 8
                  RTA_MULTIPATH, // 9
                  RTA_PROTOINFO, // no longer used
                  RTA_FLOW,      // 11
                  RTA_CACHEINFO, // 12
                  RTA_SESSION,   // no longer used
                  RTA_MP_ALGO,   // no longer used
                  RTA_TABLE,     // 15
                  RTA_MARK,      // 16
                  __RTA_MAX }; */

               switch (rtAttr->rta_type)
                  {
                  case RTA_OIF:     (void) if_indextoname(*(unsigned*)RTA_DATA(rtAttr), ifName);   break;
                  case RTA_GATEWAY: U__MEMCPY(&gateWay, RTA_DATA(rtAttr), sizeof(struct in_addr)); break;
                  case RTA_PREFSRC: U__MEMCPY(&srcAddr, RTA_DATA(rtAttr), sizeof(struct in_addr)); break;
                  case RTA_DST:     U__MEMCPY(&dstAddr, RTA_DATA(rtAttr), sizeof(struct in_addr)); break;
                  }
               }

            U_DUMP("ifName = %S dstAddr = %S rtMsg->rtm_dst_len = %u srcAddr = %S gateWay = %S", ifName,
                        UIPAddress::toString(dstAddr.s_addr).data(), rtMsg->rtm_dst_len,
                        UIPAddress::toString(srcAddr.s_addr).data(),
                        UIPAddress::toString(gateWay.s_addr).data())

            dst = U_SYSCALL(inet_ntoa, "%u", dstAddr);

            if (u__snprintf(dstMask, sizeof(dstMask), "%s/%u", dst, rtMsg->rtm_dst_len) == network_len &&
                    strncmp(dstMask, network, network_len) == 0)
               {
               if (gateWay.s_addr)
                  {
                  (void) U_SYSCALL(inet_ntop, "%d,%p,%p,%u", AF_INET, &gateWay, result.data(), result.capacity());
                  }
               else
                  {
                  U_INTERNAL_ASSERT_MAJOR(srcAddr.s_addr, 0)

                  (void) U_SYSCALL(inet_ntop, "%d,%p,%p,%u", AF_INET, &srcAddr, result.data(), result.capacity());
                  }

               result.size_adjust();

               break;
               }
            }
         }
      }
#endif

   U_RETURN_STRING(result);
}
