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
#ifdef DEBUG
            ncount = (buffer.isNull() ? 0 : buffer.space()),
#else
            ncount = buffer.space(),
#endif
            chunk  = U_max(count,(int)U_CAPACITY);

   if (ncount < chunk &&
       buffer.reserve(chunk))
      {
      ncount = buffer.space();
      }

   char* ptr = buffer.c_pointer(start);

read:
   value = s->recv(ptr + byte_read, ncount, timeoutMS);

   if (value <= 0L)
      {
      if (value == 0L) s->close(); // EOF
      else
         {
         s->checkErrno(value);

         // NB: maybe we have failed to read more bytes...

         if (byte_read &&
             s->isTimeout())
            {
            U_INTERNAL_ASSERT_EQUALS(timeoutMS,500)

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

      if (time_limit)
         {
         u_gettimeofday();

         if (timeout == 0) timeout = u_now->tv_sec + time_limit;

         // NB: may be is attacked by a "slow loris"... http://lwn.net/Articles/337853/

         if (u_now->tv_sec > timeout)
            {
            s->iState = USocket::BROKEN | USocket::TIMEOUT;

            s->close();

            result = false;

            goto done;
            }
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

      ncount = buffer.space();

      goto read;
      }

done:
   buffer.size_adjust_force(start + byte_read); // NB: we force because string can be referenced...

   U_RETURN(result);
}

// param timeoutMS specified the timeout value, in milliseconds.
// A negative value indicates no timeout, i.e. an infinite wait

// read while not received token, return position of token in buffer

uint32_t USocketExt::read(USocket* s, UString& buffer, const char* token, uint32_t token_len)
{
   U_TRACE(0, "USocketExt::read(%p,%.*S,%.*S,%u)", s, U_STRING_TO_TRACE(buffer), token_len, token, token_len)

   U_INTERNAL_ASSERT(s->isConnected())

   uint32_t count = 0, pos_token, start = buffer.size(); // il buffer di lettura potrebbe iniziare con una parte residua...

   while (USocketExt::read(s, buffer, U_SINGLE_READ, 3 * 1000))
      {
      pos_token = buffer.find(token, (start > token_len ? start - token_len - 1 : 0), token_len);

      if (pos_token != U_NOT_FOUND) U_RETURN(pos_token);

      start = buffer.size();

      // NB: attacked by a "slow loris"... http://lwn.net/Articles/337853/

      U_INTERNAL_DUMP("slow loris count = %u", count)

      if (count++ > 10) break;
      }

   U_RETURN(U_NOT_FOUND);
}

// write while sending data

bool USocketExt::write(USocket* s, const char* ptr, uint32_t ncount, int timeoutMS)
{
   U_TRACE(0, "USocketExt::write(%p,%.*S,%u,%d)", s, ncount, ptr, ncount, timeoutMS)

   U_INTERNAL_ASSERT(s->isOpen())

   ssize_t value;

   do {
      U_INTERNAL_DUMP("ncount = %u", ncount)

      value = s->send(ptr, ncount, timeoutMS);

      if (value == (ssize_t)ncount) U_RETURN(true);

      if (s->checkIO(value, ncount) == false) U_RETURN(false);

      ptr    += value;
      ncount -= value;
      }
   while (ncount);

   U_INTERNAL_ASSERT_EQUALS(ncount,0)

   U_RETURN(true);
}

bool USocketExt::write(USocket* s, const UString& header, const UString& body, int timeoutMS)
{
   U_TRACE(0, "USocketExt::write(%p,%.*S,%.*S,%d)", s, U_STRING_TO_TRACE(header), U_STRING_TO_TRACE(body), timeoutMS)

   U_INTERNAL_ASSERT(s->isOpen())

   size_t sz1 = header.size(),
          sz2 =   body.size();

   const char* ptr = header.data();

   if (sz2 == 0)
      {
      if (write(s, ptr, sz1, timeoutMS)) U_RETURN(true);

      U_RETURN(false);
      }

   ssize_t value, ncount = sz1 + sz2;

   struct iovec _iov[2] = { { (caddr_t)ptr,         sz1 },
                            { (caddr_t)body.data(), sz2 } };

loop:
   U_INTERNAL_DUMP("ncount = %u sz1 = %d", ncount, sz1)

   value = s->writev(_iov, 2, timeoutMS);

   if (value == ncount) U_RETURN(true);

   if (s->checkIO(value, ncount) == false) U_RETURN(false);

   if (sz1)
      {
      if (sz1 >= (size_t)value)
         {
         sz1             -= value;
         _iov[0].iov_base = (char*)_iov[0].iov_base + value;

         value = 0;
         }
      else
         {
         value -= sz1;
                  sz1 = 0;
         }

      _iov[0].iov_len = sz1;
      }

   _iov[1].iov_len  -= value;
   _iov[1].iov_base  = (char*)_iov[1].iov_base + value;

   ncount = sz1 + _iov[1].iov_len;

   U_INTERNAL_ASSERT_MAJOR(ncount,0)

   if (UNotifier::waitForWrite(s->iSockDesc, timeoutMS) == 1) goto loop;

   U_RETURN(false);
}

// Send a command to a server and wait for a response (single line)

int USocketExt::vsyncCommand(USocket* s, char* buffer, uint32_t buffer_size, const char* format, va_list argp)
{
   U_TRACE(0, "USocketExt::vsyncCommand(%p,%p,%u,%S)", s, buffer, buffer_size, format)

   U_INTERNAL_ASSERT(s->isOpen())

   uint32_t buffer_len = u_vsnprintf(buffer, buffer_size-2, format, argp);

   buffer[buffer_len++] = '\r';
   buffer[buffer_len++] = '\n';

   int n        = s->send(buffer, buffer_len),
       response = (s->checkIO(n, buffer_len) ? readLineReply(s, buffer, buffer_size) : (int)USocket::BROKEN);

   U_RETURN(response);
}

// Send a command to a server and wait for a response (multi line)

int USocketExt::vsyncCommandML(USocket* s, char* buffer, uint32_t buffer_size, const char* format, va_list argp)
{
   U_TRACE(0, "USocketExt::vsyncCommandML(%p,%p,%u,%S)", s, buffer, buffer_size, format)

   U_INTERNAL_ASSERT(s->isOpen())

   uint32_t buffer_len = u_vsnprintf(buffer, buffer_size-2, format, argp);

   buffer[buffer_len++] = '\r';
   buffer[buffer_len++] = '\n';

   int n        = s->send(buffer, buffer_len),
       response = (s->checkIO(n, buffer_len) ? readMultilineReply(s, buffer, buffer_size) : (int)USocket::BROKEN);

   U_RETURN(response);
}

// Send a command to a server and wait for a response (check for token line)

int USocketExt::vsyncCommandToken(USocket* s, UString& buffer, const char* format, va_list argp)
{
   U_TRACE(1, "USocketExt::vsyncCommandToken(%p,%.*S,%S)", s, U_STRING_TO_TRACE(buffer), format)

   U_INTERNAL_ASSERT(s->isOpen())
   U_ASSERT(buffer.empty() == true)

   static uint32_t cmd_count;

   char token[32];
   uint32_t token_len = u_sn_printf(token, sizeof(token), "U%04u ", cmd_count++);

   U_INTERNAL_DUMP("token = %.*S", token_len, token)

   char* p = buffer.data();

   (void) U_SYSCALL(memcpy, "%p,%p,%u", p, token, token_len);

   uint32_t buffer_len = token_len + u_vsnprintf(p+token_len, buffer.capacity(), format, argp);

   p[buffer_len++] = '\r';
   p[buffer_len++] = '\n';

   int n = s->send(p, buffer_len);

   if (s->checkIO(n, buffer_len))
      {
      uint32_t pos_token = USocketExt::read(s, buffer, token, token_len);

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

      if (i == 0 ||
          s->checkIO(i, count) == false)
         {
         U_RETURN(USocket::BROKEN);
         }

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

   (void) u_strncpy(   ifaddr.ifr_name, device, IFNAMSIZ-1);
   (void) u_strncpy(ifnetmask.ifr_name, device, IFNAMSIZ-1);

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

UString USocketExt::getNetworkInterfaceName(const UString& ip)
{
   U_TRACE(1, "USocketExt::getNetworkInterfaceName(%.*S)", U_STRING_TO_TRACE(ip))

   U_INTERNAL_ASSERT(u_isIPv4Addr(U_STRING_TO_PARAM(ip)))

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
      char _ip[16], dev[7];

      while (U_SYSCALL(fscanf, "%p,%S", arp, "%15s %*s %*s %*s %*s %6s\n", _ip, dev) != EOF)
         {
         if (ip == _ip)
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
   if (u_isIPv4Addr(device_or_ip, u_str_len(device_or_ip)))
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

      (void) u_strncpy(ifr.ifr_name, device_or_ip, IFNAMSIZ-1);

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

   (void) u_strncpy(ifr.ifr_name, device, IFNAMSIZ-1);

   /* Get the IP address of the interface */

   (void) U_SYSCALL(ioctl, "%d,%d,%p", fd, SIOCGIFADDR, &ifr);

   uusockaddr addr;

   (void) u_mem_cpy(&addr, &ifr.ifr_addr, sizeof(struct sockaddr));

   U_INTERNAL_ASSERT_EQUALS(addr.psaIP4Addr.sin_family, AF_INET)

   (void) U_SYSCALL(inet_ntop, "%d,%p,%p,%u", AF_INET, &(addr.psaIP4Addr.sin_addr), result.data(), INET_ADDRSTRLEN);

   result.size_adjust();
#endif

   U_RETURN_STRING(result);
}
