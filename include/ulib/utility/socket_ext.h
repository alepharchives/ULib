// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    socket_ext.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_SOCKET_EXT_H
#define ULIB_SOCKET_EXT_H 1

#include <ulib/string.h>
#include <ulib/net/socket.h>

class U_EXPORT USocketExt {
public:

   // SERVICES

   static UString getNodeName();
   static void    getARPCache(UVector<UString>& vec);
   static UString getNetworkInterfaceName(const UString& ip);

   static UString getIPAddress(     int fd, const char* device);        // eth0
   static UString getMacAddress(    int fd, const char* device_or_ip);  // eth0 | 192.168.1.1
   static UString getNetworkAddress(int fd, const char* device);        // eth0
   static UString getNetworkDevice(         const char* exclude);       // eth0

   // -----------------------------------------------------------------------------------------------------------------------------
   // Socket I/O
   // -----------------------------------------------------------------------------------------------------------------------------

   // manage buffered read

   static int pcount;
   static uint32_t size_message;
#ifdef DEBUG
   static char* pbuffer;
#endif

   // read while not received count data

   static bool read(USocket* s, UString& buffer, int count = U_SINGLE_READ, int timeoutMS = -1);

   // read while not received token, return position of token in buffer

   static uint32_t read(USocket* s, UString& buffer, const char* token, uint32_t token_len);

   // read while received data

   static void readEOF(USocket* s, UString& buffer)
      {
      U_TRACE(0, "USocketExt::readEOF(%p,%.*S)", s, U_STRING_TO_TRACE(buffer))

      while (USocketExt::read(s, buffer, U_SINGLE_READ)) {}
      }

   // write while sending data

   static bool write(USocket* s, const char* ptr, uint32_t count,            int timeoutMS = -1);
   static bool write(USocket* s, const UString& buffer,                      int timeoutMS = -1) { return write(s, U_STRING_TO_PARAM(buffer), timeoutMS); }
   static bool write(USocket* s, const UString& header, const UString& body, int timeoutMS = -1);

   // Send a command to a server and wait for a response (single line)

   static int vsyncCommand(USocket* s, char* buffer, uint32_t buffer_size, const char* format, va_list argp);

   static int vsyncCommand(USocket* s, const char* format, va_list argp)
      {
      U_TRACE(0, "USocketExt::vsyncCommand(%p,%S)", s, format)

      U_INTERNAL_ASSERT_EQUALS(u_buffer_len,0)

      return vsyncCommand(s, u_buffer, sizeof(u_buffer), format, argp);
      }

   // response from server (single line)

   static int readLineReply(USocket* s, char* buffer, uint32_t buffer_size);

   static int readLineReply(USocket* s)
      {
      U_TRACE(0, "USocketExt::readLineReply(%p)", s)

      U_INTERNAL_ASSERT_EQUALS(u_buffer_len,0)

      return readLineReply(s, u_buffer, sizeof(u_buffer));
      }

   static int readLineReply(USocket* s, UString& buffer)
      {
      U_TRACE(0, "USocketExt::readLineReply(%p,%p)", s, &buffer)

      int n = readLineReply(s, buffer.data(), buffer.capacity());

      buffer.size_adjust(U_max(n,0));

      U_RETURN(n);
      }

   // Send a command to a server and wait for a response (multi line)

   static int vsyncCommandML(USocket* s, char* buffer, uint32_t buffer_size, const char* format, va_list argp);

   static int vsyncCommandML(USocket* s, const char* format, va_list argp)
      {
      U_TRACE(0, "USocketExt::vsyncCommandML(%p,%S)", s, format)

      U_INTERNAL_ASSERT_EQUALS(u_buffer_len,0)

      return vsyncCommandML(s, u_buffer, sizeof(u_buffer), format, argp);
      }

   // response from server (multi line)

   static int readMultilineReply(USocket* s, char* buffer, uint32_t buffer_size);

   static int readMultilineReply(USocket* s)
      {
      U_TRACE(0, "USocketExt::readMultilineReply(%p)", s)

      U_INTERNAL_ASSERT_EQUALS(u_buffer_len,0)

      return readMultilineReply(s, u_buffer, sizeof(u_buffer));
      }

   // Send a command to a server and wait for a response (check for token line)

   static int vsyncCommandToken(USocket* s, UString& buffer, const char* format, va_list argp);
   // -----------------------------------------------------------------------------------------------------------------------------

private:
   static inline bool parseCommandResponse(char* buffer, int r, int response) U_NO_EXPORT;
};

#endif
