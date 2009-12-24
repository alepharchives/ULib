// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    ssl_ftp.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef U_SSL_FTPCLIENT_H
#define U_SSL_FTPCLIENT_H 1

#include <ulib/net/client/ftp.h>
#include <ulib/ssl/net/sslsocket.h>

/**
   @class USSLFtpClient

   @brief Creates and manages a secure client connection with a remote FTP server.

   Establishing a protected session:

           Client                                 Server
  control          data                   data               control
====================================================================
                                                             socket()
                                                             bind()
  socket()
  connect()  ----------------------------------------------> accept()
            <----------------------------------------------  220
  AUTH TLS   ---------------------------------------------->
            <----------------------------------------------  234
  TLSneg()  <----------------------------------------------> TLSneg()
  PBSZ 0     ---------------------------------------------->
            <----------------------------------------------  200
  PROT P     ---------------------------------------------->
            <----------------------------------------------  200
  USER fred  ---------------------------------------------->
            <----------------------------------------------  331
  PASS pass  ---------------------------------------------->
            <----------------------------------------------  230
*/

class U_EXPORT USSLFtpClient : public UFtpClient {
public:

   /**
   Constructs a new USSLFtpClient with default values for all properties.
   */

   USSLFtpClient(bool bSocketIsIPv6 = false) : UFtpClient(bSocketIsIPv6)
      {
      U_TRACE_REGISTER_OBJECT(0, USSLFtpClient, "%b", bSocketIsIPv6)
      }

   virtual ~USSLFtpClient();

   /**
   This method is to be called after connectServer() and before login() to secure the ftp communication channel.

   @returns @c true if successful and @c false if the ssl negotiation failed

   Notes: The library uses an ssl/tls encryption approach defined in the draft-murray-auth-ftp-ssl
          available at http://www.ford-hutchinson.com/~fh-1-pfh/ftps-ext.html.
   */

   bool negotiateEncryption();

   /**
   On an already secured ftp session, setDataEncryption() specifies if the data connection
   channel will be secured for the next data transfer.

   @param @secure flag either unencrypted (=false) or secure (=true)
   @returns @c true if successful and @c false if the control connection isn't secure or on error
   */

   bool setDataEncryption(bool secure = true);

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const { return UFtpClient::dump(reset); }
#endif

private:
   USSLFtpClient(const USSLFtpClient&) : UFtpClient(false) {}
   USSLFtpClient& operator=(const USSLFtpClient&)          { return *this; }
};

#endif
