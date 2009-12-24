// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    ssl_ftp.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/ssl/net/client/ssl_ftp.h>

USSLFtpClient::~USSLFtpClient()
{
   U_TRACE_UNREGISTER_OBJECT(0, USSLFtpClient)

   if (tlsctrl)
      {
      // NB: we use secureConnection()...

      tlsctrl->USocket::iState    = CLOSE;
      tlsctrl->USocket::iSockDesc = -1;

      delete tlsctrl;
      }

   if (tlsdata)
      {
      // NB: we use secureConnection()...

      tlsdata->USocket::iState    = CLOSE;
      tlsdata->USocket::iSockDesc = -1;

      delete tlsdata;
      }
}

bool USSLFtpClient::negotiateEncryption()
{
   U_TRACE(0, "USSLFtpClient::negotiateEncryption()")

   U_INTERNAL_ASSERT_EQUALS(tlsctrl,0)

   if (syncCommand("AUTH TLS") &&
       response == 234)
      {
      tlsctrl = U_NEW(USSLSocket(USocket::bIPv6Socket, 0));

      if (tlsctrl->secureConnection(USocket::getFd())) U_RETURN(true);
      }

   U_RETURN(false);
}

bool USSLFtpClient::setDataEncryption(bool secure)
{
   U_TRACE(0, "USSLFtpClient::setDataEncryption(%b)", secure)

   U_INTERNAL_ASSERT_POINTER(tlsctrl)

   if (syncCommand("PBSZ 0")                        && response == FTP_COMMAND_OK &&
       syncCommand("PROT %c", (secure ? 'P' : 'C')) && response == FTP_COMMAND_OK)
      {
      if (secure) tlsdata = U_NEW(USSLSocket(USocket::bIPv6Socket, tlsctrl->ctx));

      U_RETURN(true);
      }

   U_RETURN(false);
}
