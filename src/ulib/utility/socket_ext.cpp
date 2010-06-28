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

#include <ulib/notifier.h>
#include <ulib/container/vector.h>
#include <ulib/utility/socket_ext.h>

#ifdef __MINGW32__
#  include <ws2tcpip.h>
#elif defined(HAVE_NETPACKET_PACKET_H) && !defined(U_ALL_CPP)
#  include <net/if.h>
#  include <sys/ioctl.h>
#endif

int      USocketExt::pcount;
uint32_t USocketExt::size_message;
#ifdef DEBUG
char*    USocketExt::pbuffer;
#endif

// Socket I/O

// param timeoutMS specified the timeout value, in milliseconds.
// A negative value indicates no timeout, i.e. an infinite wait

bool USocketExt::read(USocket* socket, UString& buffer, int count, int timeoutMS) // read while not received count data
{
   U_TRACE(0, "USocketExt::read(%p,%.*S,%d,%d)", socket, U_STRING_TO_TRACE(buffer), count, timeoutMS)

   U_INTERNAL_ASSERT_DIFFERS(count,0)
   U_INTERNAL_ASSERT(socket->isConnected())

   char* ptr;
   ssize_t value;
   uint32_t start;
   bool single_read;
   int diff, byte_read;

   // NB: THINK REALLY VERY MUCH BEFORE CHANGE CODE HERE...

restart:
   start       = buffer.size(); // il buffer di lettura potrebbe iniziare con una parte residua
   byte_read   = 0;
   single_read = (count == U_SINGLE_READ);

   // manage buffered read

   if (pcount > 0) // NB: va bene cosi'.... (la size di un argomento di RPC puo' essere anche di un byte...)
      {
      U_INTERNAL_DUMP("pcount = %d pbuffer = %p buffer = %p", pcount, pbuffer, buffer.data())

      U_INTERNAL_ASSERT_EQUALS(buffer.data(), pbuffer)

      if (single_read) count = pcount;

      diff = pcount - count;

      U_INTERNAL_DUMP("diff = %d", diff)

      if (diff >= 0)
         {
         // NB: NO read()...!!!

         pcount -= (byte_read = count);

         U_INTERNAL_DUMP("pcount = %d", pcount)

         goto done;
         }

      // diff < 0

      buffer.size_adjust_force(start + pcount); // NB: we force for U_SUBSTR_INC_REF case (string can be referenced more)...

      count -= pcount;
      pcount = 0;

      if (count < (int)U_CAPACITY) count = U_SINGLE_READ; // NB: may be we can read more bytes then required...

      U_INTERNAL_DUMP("count = %d", count)

      goto restart;
      }

   if (count < (int)U_CAPACITY) single_read = true;

   (void) buffer.reserve(start + (single_read ? (int)U_CAPACITY : count));

   ptr = buffer.c_pointer(start);

   U_INTERNAL_DUMP("start = %u single_read = %b count = %d", start, single_read, count)

read:
   value = socket->recv(ptr + byte_read, (single_read ? (int)U_CAPACITY : count - byte_read));

   if (value <= 0)
      {
      if (value == 0) socket->close(); // EOF
      else
         {
         socket->checkErrno(value);

         if (timeoutMS != -1     &&
             socket->isTimeout() &&
             UNotifier::waitForRead(socket->getFd(), timeoutMS) == 1)
            {
            socket->iState = USocket::CONNECT;

            goto read;
            }
         }

      U_RETURN(false);
      }

   byte_read += value;

   U_INTERNAL_DUMP("byte_read = %d", byte_read)

   if (byte_read < count) goto read;

   if (single_read)
      {
      // NB: may be there are available more bytes to read...

      if (value == U_CAPACITY)
         {
         buffer.size_adjust_force(start + byte_read); // NB: we force for U_SUBSTR_INC_REF case (string can be referenced more)...

         if (buffer.reserve(start + byte_read + U_CAPACITY)) ptr = buffer.c_pointer(start);

         goto read;
         }

      if (count != U_SINGLE_READ)
         {
         // NB: may be we have read more bytes then required...

         pcount = (byte_read - count); // NB: here pcount is always == 0...

         if (pcount >= 0)
            {
            byte_read = count;
#        ifdef DEBUG
            pbuffer   = buffer.data();
#        endif
            }

         U_INTERNAL_DUMP("byte_read = %d count = %d pcount = %d", byte_read, count, pcount)
         }
      }

done:
   buffer.size_adjust_force(start + byte_read); // NB: we force for U_SUBSTR_INC_REF case (string can be referenced more)...

   if (count != U_SINGLE_READ)
      {
      size_message += count;

      U_INTERNAL_DUMP("size_message = %u", size_message)
      }

   U_RETURN(true);
}

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

      if (count++ > 10) break;
      }

   U_RETURN(U_NOT_FOUND);
}

// write while sending data

bool USocketExt::write(USocket* s, const char* ptr, uint32_t count, int timeoutMS)
{
   U_TRACE(0, "USocketExt::write(%p,%.*S,%u,%d)", s, count, ptr, count, timeoutMS)

   U_INTERNAL_ASSERT(s->isOpen())

   ssize_t value;
   bool write_again;

   do {
      U_INTERNAL_DUMP("count = %u", count)

      value = s->send(ptr, count);

      if (s->checkIO(value, count) == false)
         {
         if (timeoutMS != -1 &&
             s->isTimeout()  &&
             UNotifier::waitForWrite(s->getFd(), timeoutMS) == 1)
            {
            s->iState = USocket::CONNECT;

            continue;
            }

         U_RETURN(false);
         }

      write_again = (value != (ssize_t)count);

      ptr   += value;
      count -= value;
      }
   while (write_again);

   U_INTERNAL_ASSERT_EQUALS(count,0)

   U_RETURN(true);
}

bool USocketExt::write(USocket* s, const UString& header, const UString& body, int timeoutMS)
{
   U_TRACE(0, "USocketExt::write(%p,%.*S,%.*S,%d)", s, U_STRING_TO_TRACE(header), U_STRING_TO_TRACE(body), timeoutMS)

   U_INTERNAL_ASSERT(s->isOpen())

   int sz1 = header.size(),
       sz2 =   body.size();

   ssize_t value, count = sz1 + sz2;

   struct iovec iov[2] = { { (caddr_t)header.data(), sz1 },
                           { (caddr_t)  body.data(), sz2 } };

   while (true)
      {
      U_INTERNAL_DUMP("count = %u", count)

      value = s->writev(iov, 2);

      if (s->checkIO(value, count) == false)
         {
         if (timeoutMS != -1 &&
             s->isTimeout()  &&
             UNotifier::waitForWrite(s->getFd(), timeoutMS) == 1)
            {
            s->iState = USocket::CONNECT;

            continue;
            }

         U_RETURN(false);
         }

      if (value == count) break;

      if (sz1)
         {
         if (sz1 >= value)
            {
            sz1             -= value;
            iov[0].iov_base  = (char*)iov[0].iov_base + value;

            value = 0;
            }
         else
            {
            value -= sz1;
                     sz1 = 0;
            }

         iov[0].iov_len = sz1;
         }

      iov[1].iov_len  -= value;
      iov[1].iov_base  = (char*)iov[1].iov_base + value;

      count = sz1 + iov[1].iov_len;

      U_INTERNAL_ASSERT_MAJOR(count,0)
      }

   U_RETURN(true);
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
   uint32_t token_len = u_snprintf(token, sizeof(token), "U%04u ", cmd_count++);

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
#     ifdef DEBUG
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

#ifndef __MINGW32__
   bool found;
   char dev[7], dest[9];
   FILE* route = (FILE*) U_SYSCALL(fopen, "%S,%S", "/proc/net/route", "r");

   // Skip first line

   if (U_SYSCALL(fscanf, "%p,%S", route, "%*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s", 0) != EOF)
      {
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
#ifndef __MINGW32__
   struct ifreq ifaddr, ifnetmask;

   (void) strncpy(   ifaddr.ifr_name, device, IFNAMSIZ-1);
   (void) strncpy(ifnetmask.ifr_name, device, IFNAMSIZ-1);

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

UString USocketExt::getMacAddress(int fd, const char* device_or_ip)
{
   U_TRACE(1, "USocketExt::getMacAddress(%d,%S)", fd, device_or_ip)

   U_INTERNAL_ASSERT_POINTER(device_or_ip)

   UString result(100U);

#ifndef __MINGW32__
   if (u_isIPv4Addr(device_or_ip, u_strlen(device_or_ip)))
      {
      FILE* arp = (FILE*) U_SYSCALL(fopen, "%S,%S", "/proc/net/arp", "r");

      // ------------------------------------------------------------------------------
      // Skip the first line
      // ------------------------------------------------------------------------------
      // IP address       HW type     Flags       HW address            Mask     Device
      // 192.168.253.1    0x1         0x2         00:14:a5:6e:9c:cb     *        ath0
      // 10.30.1.131      0x1         0x2         00:16:ec:fb:46:da     *        eth0
      // ------------------------------------------------------------------------------

      if (U_SYSCALL(fscanf, "%p,%S", arp, "%*s %*s %*s %*s %*s %*s %*s %*s %*s", 0) != EOF)
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

      (void) strncpy(ifr.ifr_name, device_or_ip, IFNAMSIZ-1);

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

UString USocketExt::getNetworkInterfaceName(const UString& ip)
{
   U_TRACE(1, "USocketExt::getNetworkInterfaceName(%.*S)", U_STRING_TO_TRACE(ip))

   U_INTERNAL_ASSERT(u_isIPv4Addr(U_STRING_TO_PARAM(ip)))

   UString result(100U);

#ifndef __MINGW32__
   FILE* arp = (FILE*) U_SYSCALL(fopen, "%S,%S", "/proc/net/arp", "r");

   // ------------------------------------------------------------------------------
   // Skip the first line
   // ------------------------------------------------------------------------------
   // IP address       HW type     Flags       HW address            Mask     Device
   // 192.168.253.1    0x1         0x2         00:14:a5:6e:9c:cb     *        ath0
   // 10.30.1.131      0x1         0x2         00:16:ec:fb:46:da     *        eth0
   // ------------------------------------------------------------------------------

   if (U_SYSCALL(fscanf, "%p,%S", arp, "%*s %*s %*s %*s %*s %*s %*s %*s %*s", 0) != EOF)
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

void USocketExt::getARPCache(UVector<UString>& vec)
{
   U_TRACE(1, "USocketExt::getARPCache(%p)", &vec)

#ifndef __MINGW32__
   FILE* arp = (FILE*) U_SYSCALL(fopen, "%S,%S", "/proc/net/arp", "r");

   // ------------------------------------------------------------------------------
   // Skip the first line
   // ------------------------------------------------------------------------------
   // IP address       HW type     Flags       HW address            Mask     Device
   // 192.168.253.1    0x1         0x2         00:14:a5:6e:9c:cb     *        ath0
   // 10.30.1.131      0x1         0x2         00:16:ec:fb:46:da     *        eth0
   // ------------------------------------------------------------------------------

   if (U_SYSCALL(fscanf, "%p,%S", arp, "%*s %*s %*s %*s %*s %*s %*s %*s %*s", 0) != EOF)
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

UString USocketExt::getIPAddress(int fd, const char* device)
{
   U_TRACE(1, "USocketExt::getIPAddress(%d,%S)", fd, device)

   U_INTERNAL_ASSERT(fd != -1)
   U_INTERNAL_ASSERT_POINTER(device)

   UString result(100U);

#ifndef __MINGW32__
   struct ifreq ifr;

   (void) strncpy(ifr.ifr_name, device, IFNAMSIZ-1);

   (void) U_SYSCALL(ioctl, "%d,%d,%p", fd, SIOCGIFHWADDR, &ifr);

   /* Get the IP address of the interface */

   union uusockaddr {
      struct sockaddr*     psaGeneric;
      struct sockaddr_in*  psaIP4Addr;
   };

   union uusockaddr psaGeneric = { &(ifr.ifr_addr) };

   psaGeneric.psaIP4Addr->sin_family = AF_INET;

   (void) U_SYSCALL(ioctl, "%d,%d,%p", fd, SIOCGIFADDR, &ifr);

   (void) U_SYSCALL(inet_ntop, "%d,%p,%p,%u", AF_INET, &(psaGeneric.psaIP4Addr->sin_addr), result.data(), INET_ADDRSTRLEN);

   result.size_adjust();
#endif

   U_RETURN_STRING(result);
}

UString USocketExt::getNodeName()
{
   U_TRACE(0, "USocketExt::getNodeName()")

#ifndef __MINGW32__
   /*
   UString result(100U);
   */

   /* 1
   struct utsname buf;
   if (U_SYSCALL(uname, "%p", &buf) == 0) (void) result.assign(buf.nodename);
   */

   /* 2
   FILE* node = (FILE*) U_SYSCALL(fopen, "%S,%S", "/proc/sys/kernel/hostname", "r");
   if (U_SYSCALL(fscanf, "%p,%S", node, "%*s", result.data()) != EOF) result.size_adjust();
   (void) U_SYSCALL(fclose, "%p", node);
   */
#endif

   UString result(u_hostname, u_hostname_len);

   U_RETURN_STRING(result);
}
