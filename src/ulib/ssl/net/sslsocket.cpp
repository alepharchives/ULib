// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    sslsocket.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/utility/services.h>
#include <ulib/ssl/net/sslsocket.h>

#include <openssl/err.h>

#ifndef SSL_ERROR_WANT_ACCEPT
#define SSL_ERROR_WANT_ACCEPT SSL_ERROR_WANT_READ
#endif

SSL_METHOD* USSLSocket::method;

USSLSocket::USSLSocket(bool bSocketIsIPv6, SSL_CTX* _ctx, bool tls) : UTCPSocket(bSocketIsIPv6)
{
   U_TRACE_REGISTER_OBJECT(0, USSLSocket, "%b,%p,%b", bSocketIsIPv6, _ctx, tls)

   // init new generic CTX object as framework to establish TLS/SSL enabled connections

   if (_ctx)
      {
      ctx = _ctx;

      ctx->references++; // We don't want our destructor to delete ctx if still in use...
      }
   else
      {
      if (method == 0) method = (SSL_METHOD*) (tls ? TLSv1_method() : SSLv3_method());

      ctx = (SSL_CTX*) U_SYSCALL(SSL_CTX_new, "%p", method);

      (void) U_SYSCALL(SSL_CTX_set_mode,    "%p,%d", ctx, SSL_MODE_ENABLE_PARTIAL_WRITE | SSL_MODE_AUTO_RETRY);
      (void) U_SYSCALL(SSL_CTX_set_options, "%p,%d", ctx, SSL_OP_NO_SSLv2);
      }

   U_INTERNAL_ASSERT_POINTER(ctx)

   ret    = 0;
   ssl    = 0;
   active = true;
}

USSLSocket::~USSLSocket()
{
   U_TRACE_UNREGISTER_OBJECT(0, USSLSocket)

   if (ssl) U_SYSCALL_VOID(SSL_free, "%p", ssl); /* SSL_free will free UServices::store */

   if (ctx) U_SYSCALL_VOID(SSL_CTX_free, "%p", ctx);
}

bool USSLSocket::useDHFile(const char* dh_file)
{
   U_TRACE(1, "USSLSocket::useDHFile(%S)", dh_file)

   // Set up DH stuff

   DH* dh;

   if (dh_file)
      {
      FILE* paramfile = (FILE*) U_SYSCALL(fopen, "%S,%S", dh_file, "r");

      if (paramfile == 0) U_RETURN(false);

      dh = (DH*) U_SYSCALL(PEM_read_DHparams, "%p,%p,%p,%p", paramfile, 0, 0, 0);

      (void) U_SYSCALL(fclose, "%p", paramfile);
      }
   else
      {
      static unsigned char dh1024_p[] = {
         0xF8,0x81,0x89,0x7D,0x14,0x24,0xC5,0xD1,0xE6,0xF7,0xBF,0x3A,
         0xE4,0x90,0xF4,0xFC,0x73,0xFB,0x34,0xB5,0xFA,0x4C,0x56,0xA2,
         0xEA,0xA7,0xE9,0xC0,0xC0,0xCE,0x89,0xE1,0xFA,0x63,0x3F,0xB0,
         0x6B,0x32,0x66,0xF1,0xD1,0x7B,0xB0,0x00,0x8F,0xCA,0x87,0xC2,
         0xAE,0x98,0x89,0x26,0x17,0xC2,0x05,0xD2,0xEC,0x08,0xD0,0x8C,
         0xFF,0x17,0x52,0x8C,0xC5,0x07,0x93,0x03,0xB1,0xF6,0x2F,0xB8,
         0x1C,0x52,0x47,0x27,0x1B,0xDB,0xD1,0x8D,0x9D,0x69,0x1D,0x52,
         0x4B,0x32,0x81,0xAA,0x7F,0x00,0xC8,0xDC,0xE6,0xD9,0xCC,0xC1,
         0x11,0x2D,0x37,0x34,0x6C,0xEA,0x02,0x97,0x4B,0x0E,0xBB,0xB1,
         0x71,0x33,0x09,0x15,0xFD,0xDD,0x23,0x87,0x07,0x5E,0x89,0xAB,
         0x6B,0x7C,0x5F,0xEC,0xA6,0x24,0xDC,0x53 };
      static unsigned char dh1024_g[] = { 0x02 };

      dh = (DH*) U_SYSCALL_NO_PARAM(DH_new);

      dh->p = BN_bin2bn(dh1024_p, sizeof(dh1024_p), 0);
      dh->g = BN_bin2bn(dh1024_g, sizeof(dh1024_g), 0);

      U_INTERNAL_ASSERT_POINTER(dh->g)
      U_INTERNAL_ASSERT_POINTER(dh->p)
      }

   if (dh == 0) U_RETURN(false);

   /*
#ifdef DEBUG
   unsigned char buf[512];

   int len  = i2d_DHparams(dh, 0),
       size = i2d_DHparams(dh, (unsigned char**)&buf);

   U_INTERNAL_DUMP("len = %d buf(%d) = %#.*S", len, size, size, buf)
#endif
   */

   U_INTERNAL_ASSERT_POINTER(ctx)

   (void) U_SYSCALL(SSL_CTX_set_tmp_dh, "%p,%p", ctx, dh);

   U_SYSCALL_VOID(DH_free, "%p", dh);

   U_RETURN(true);
}

bool USSLSocket::setContext(const char* cert_file,
                            const char* private_key_file, const char* passwd,
                            const char* CAfile, const char* CApath, int mode)
{
   U_TRACE(1, "USSLSocket::setContext(%S,%S,%S,%S,%S,%d)", cert_file, private_key_file, passwd, CAfile, CApath, mode)

   U_INTERNAL_ASSERT_POINTER(ctx)

   int result;

   // Load CERT PEM file

   if (cert_file && *cert_file)
      {
      result = U_SYSCALL(SSL_CTX_use_certificate_chain_file, "%p,%S", ctx, cert_file);

      if (result == 0) U_RETURN(false);
      }

   // Load private key PEM file and give passwd callback if any

   if (private_key_file && *private_key_file)
      {
      if (passwd && *passwd)
         {
         U_SYSCALL_VOID(SSL_CTX_set_default_passwd_cb,          "%p,%p", ctx, u_passwd_cb);
         U_SYSCALL_VOID(SSL_CTX_set_default_passwd_cb_userdata, "%p,%S", ctx, (void*)passwd);
         }

      for (int i = 0; i < 3; ++i)
         {
         result = U_SYSCALL(SSL_CTX_use_PrivateKey_file, "%p,%S,%d", ctx, private_key_file, SSL_FILETYPE_PEM);

         if (result) break;

         unsigned long error = U_SYSCALL_NO_PARAM(ERR_peek_error);

         if (ERR_GET_REASON(error) == EVP_R_BAD_DECRYPT)
            {
            if (i < 2) // Give the user two tries
               {
               (void) U_SYSCALL_NO_PARAM(ERR_get_error); // remove from stack

               continue;
               }

            U_RETURN(false);
            }

         U_RETURN(false);
         }

      // Check private key

      result = U_SYSCALL(SSL_CTX_check_private_key, "%p", ctx);

      if (result == 0) U_RETURN(false);
      }

   if (CAfile && *CAfile == '\0') CAfile = 0;
   if (CApath && *CApath == '\0') CApath = 0;

   if (CAfile || CApath)
      {
      /*
      if (CAfile)
         {
         result = U_SYSCALL(SSL_CTX_load_verify_locations, "%p,%S,%S", ctx, CAfile, 0);

         if (result == 0) U_RETURN(false);
         }

      if (CApath)
         {
         result = U_SYSCALL(SSL_CTX_load_verify_locations, "%p,%S,%S", ctx, 0, CApath);

         if (result == 0) U_RETURN(false);

         UServices::setCApath(CApath);
         }
      */

      if (UServices::setupOpenSSLStore(CAfile, CApath) == false) U_RETURN(false);

      U_SYSCALL_VOID(SSL_CTX_set_cert_store, "%p,%p", ctx, UServices::store);

      /* Sets the list of CAs sent to the client when requesting a client certificate for ctx */

      if (CAfile) // Process CA certificate bundle file
         {
         STACK_OF(X509_NAME)* list = (STACK_OF(X509_NAME)*) U_SYSCALL(SSL_load_client_CA_file, "%S", CAfile);

         U_SYSCALL_VOID(SSL_CTX_set_client_CA_list, "%p,%p", ctx, list);
         }
      }

   setVerifyCallback(UServices::X509Callback, mode);

   static int s_server_session_id_context = 1;

   ret = U_SYSCALL(SSL_CTX_set_session_id_context, "%p,%p,%u", ctx,
                     (const unsigned char*)&s_server_session_id_context, sizeof(s_server_session_id_context));

   if (ret != 1)
      {
      U_DUMP("status = %.*S", 512, status(true))

      U_RETURN(false);
      }

   U_RETURN(true);
}

const char* USSLSocket::status(SSL* ssl, int ret, bool flag)
{
   U_TRACE(1, "USSLSocket::status(%p,%d,%b)", ssl, ret, flag)

   const char* descr  = "SSL_ERROR_NONE";
   const char* errstr = "ok";

   if (ret != SSL_ERROR_NONE) // 0
      {
      if (flag) ret = U_SYSCALL(SSL_get_error, "%p,%d", ssl, ret);

      /* -------------------------------------
       * #define SSL_ERROR_NONE              0
       * #define SSL_ERROR_SSL               1
       * #define SSL_ERROR_WANT_READ         2
       * #define SSL_ERROR_WANT_WRITE        3
       * #define SSL_ERROR_WANT_X509_LOOKUP  4
       * #define SSL_ERROR_SYSCALL           5
       * #define SSL_ERROR_ZERO_RETURN       6
       * #define SSL_ERROR_WANT_CONNECT      7
       * #define SSL_ERROR_WANT_ACCEPT       8
       * -------------------------------------
       */

      switch (ret)
         {
         case SSL_ERROR_SSL:
            {
            descr  = "SSL_ERROR_SSL";
            errstr = "SSL error";
            }
         break;

         case SSL_ERROR_SYSCALL:
            {
            descr  = "SSL_ERROR_SYSCALL";
            errstr = "SSL EOF observed that violates the protocol";
            }
         break;

         case SSL_ERROR_ZERO_RETURN:
            {
            descr  = "SSL_ERROR_ZERO_RETURN";
            errstr = "SSL connection closed by peer";
            }
         break;

         case SSL_ERROR_WANT_X509_LOOKUP:
            {
            descr  = "SSL_ERROR_WANT_X509_LOOKUP";
            errstr = "SSL operation didn't complete, the same function should be called again later";
            }
         break;

         case SSL_ERROR_WANT_READ:
            {
            descr  = "SSL_ERROR_WANT_READ";
            errstr = "SSL Read operation didn't complete, the same function should be called again later";
            }
         break;

         case SSL_ERROR_WANT_WRITE:
            {
            descr  = "SSL_ERROR_WANT_WRITE";
            errstr = "SSL Write operation didn't complete, the same function should be called again later";
            }
         break;

         case SSL_ERROR_WANT_CONNECT:
            {
            descr  = "SSL_ERROR_WANT_CONNECT";
            errstr = "SSL Connect operation didn't complete, the same function should be called again later";
            }
         break;

         case SSL_ERROR_WANT_ACCEPT:
            {
            descr  = "SSL_ERROR_WANT_ACCEPT";
            errstr = "SSL Accept operation didn't complete, the same function should be called again later";
            }
         break;
         }
      }

   U_INTERNAL_ASSERT_EQUALS(u_buffer_len,0)

   u_buffer_len = u_snprintf(u_buffer, sizeof(u_buffer), "(%d, %s) - %s", ret, descr, errstr);

   uint32_t len;

   char* sslerr = UServices::getOpenSSLError(0, 0, &len);

   if (len)
      {
      u_buffer[u_buffer_len++] = ' ';

      (void) U_SYSCALL(memcpy, "%p,%p,%u", (void*)(u_buffer + u_buffer_len), sslerr, len+1);

      u_buffer_len += len;
      }

   U_RETURN(u_buffer);
}

void USSLSocket::getMsgError(const char*& msg)
{
   U_TRACE(0, "USSLSocket::getMsgError(%p)", &msg)

   U_INTERNAL_DUMP("ret = %d", ret)

   if (ret == SSL_ERROR_NONE) USocket::getMsgError(msg);
   else
      {
      if (u_buffer_len == 0) (void) status(false);

      msg = u_buffer;

      u_buffer_len = 0;
      }
}

bool USSLSocket::secureConnection(int fd)
{
   U_TRACE(1, "USSLSocket::secureConnection(%d)", fd)

   if  (ssl)  (void) U_SYSCALL(SSL_clear, "%p", ssl); // reuse old
   else ssl = (SSL*) U_SYSCALL(SSL_new,   "%p", ctx);

   // When beginning a new handshake, the SSL engine must know whether it must call the connect (client) or accept (server) routines.
   // Even though it may be clear from the method chosen, whether client or server mode was requested, the handshake routines must be explicitly set.
   // U_SYSCALL_VOID(SSL_set_connect_state, "%p", ssl); // init SSL client session

   ret = 0;

   if (U_SYSCALL(SSL_set_fd,  "%p,%d", ssl, fd)) // get SSL to use our socket
      {
loop:
      ret = U_SYSCALL(SSL_connect, "%p", ssl); // get SSL to handshake with server

      if (ret == 1)
         {
         USocket::iState    = CONNECT;
         USocket::iSockDesc = fd; // used by USmtpClient class...

         U_RETURN(true);
         }

      ret = U_SYSCALL(SSL_get_error, "%p,%d", ssl, ret);

      U_DUMP("status = %.*S", 512, status(false))

      if (ret == SSL_ERROR_WANT_READ  ||
          ret == SSL_ERROR_WANT_WRITE ||
          ret == SSL_ERROR_WANT_CONNECT) goto loop;
      }

   U_RETURN(false);
}

// server side RE-NEGOTIATE asking for client cert

bool USSLSocket::askForClientCertificate()
{
   U_TRACE(1, "USSLSocket::askForClientCertificate()")

   /*
   SSL_VERIFY_CLIENT_ONCE
   -------------------------------------------------------------------------------------
   Client mode: ignored
   Server mode: only request a client certificate on the initial TLS/SSL handshake.
                Do not ask for a client certificate again in case of a renegotiation.
                This flag must be used together with SSL_VERIFY_PEER.
   -------------------------------------------------------------------------------------
   */

   U_SYSCALL_VOID(SSL_set_verify, "%p,%d,%p", ssl, SSL_VERIFY_PEER_STRICT, NULL); // | SSL_VERIFY_CLIENT_ONCE

   /* Stop the client from just resuming the un-authenticated session */

   static int s_server_auth_session_id_context = 2;

   ret = U_SYSCALL(SSL_set_session_id_context, "%p,%p,%u", ssl,
                     (const unsigned char*)&s_server_auth_session_id_context, sizeof(s_server_auth_session_id_context));

   if (ret != 1)
      {
      U_DUMP("status = %.*S", 512, status(true))

      U_RETURN(false);
      }

   ret = U_SYSCALL(SSL_renegotiate, "%p", ssl);

   if (ret != 1)
      {
      U_DUMP("status = %.*S", 512, status(true))

      U_RETURN(false);
      }

   ret = U_SYSCALL(SSL_do_handshake, "%p", ssl);

   if (ret != 1)
      {
      U_DUMP("status = %.*S", 512, status(true))

      U_RETURN(false);
      }

   ssl->state = SSL_ST_ACCEPT;

   ret = U_SYSCALL(SSL_do_handshake, "%p", ssl);

   if (ret != 1)
      {
      U_DUMP("status = %.*S", 512, status(true))

      U_RETURN(false);
      }

   U_RETURN(true);
}

// VIRTUAL METHOD

void USSLSocket::closesocket()
{
   U_TRACE(1, "USSLSocket::closesocket()")

   if (ssl)
      {
      ret = U_SYSCALL(SSL_shutdown, "%p", ssl); // Send SSL shutdown signal to peer

      U_DUMP("status = %.*S", 512, status(true))

      U_SYSCALL_VOID(SSL_set_shutdown, "%p,%d", ssl, SSL_SENT_SHUTDOWN | SSL_RECEIVED_SHUTDOWN);

      U_SYSCALL_VOID(SSL_free, "%p", ssl);

      ssl = 0;                
      }

   USocket::closesocket();
}

USocket* USSLSocket::accept(USSLSocket* pcNewConnection)
{
   U_TRACE(1, "USSLSocket::accept(%p)", pcNewConnection)

   bool reuse = (ssl != 0);

   U_INTERNAL_DUMP("reuse = %b", reuse)

retry:
   if (reuse) (void) U_SYSCALL(SSL_clear, "%p", ssl); // reuse old
   else ssl = (SSL*) U_SYSCALL(SSL_new,   "%p", ctx);

   // --------------------------------------------------------------------------------------------------
   // When beginning a new handshake, the SSL engine must know whether it must call the connect (client)
   // or accept (server) routines. Even though it may be clear from the method chosen, whether client or
   // server mode was requested, the handshake routines must be explicitly set.
   // --------------------------------------------------------------------------------------------------
   // U_SYSCALL_VOID(SSL_set_accept_state, "%p", ssl); // init SSL server session
   // --------------------------------------------------------------------------------------------------

   ret = 0;

   if (U_SYSCALL(SSL_set_fd, "%p,%d", ssl, pcNewConnection->iSockDesc)) // get SSL to use our socket
      {
loop:
      errno = 0;
      ret   = U_SYSCALL(SSL_accept, "%p", ssl); // get SSL handshake with client

      if (ret == 1)
         {
         pcNewConnection->ssl = ssl;
         pcNewConnection->ret = SSL_ERROR_NONE;

         ssl = 0;

         goto end;
         }

      U_INTERNAL_DUMP("errno = %d", errno)

      pcNewConnection->ret = U_SYSCALL(SSL_get_error, "%p,%d", ssl, ret);

      U_DUMP("status = %.*S", 512, (u_buffer_len = 0, status(ssl, pcNewConnection->ret, false)))

      if (USocket::req_timeout == 0 &&
          (pcNewConnection->ret == SSL_ERROR_WANT_READ  ||
           pcNewConnection->ret == SSL_ERROR_WANT_WRITE ||
           pcNewConnection->ret == SSL_ERROR_WANT_ACCEPT)) goto loop;
      }

   if (ret != 1)
      {
      if (reuse)
         {
         U_DUMP("status = %.*S", 512, status(true))

         U_SYSCALL_VOID(SSL_free, "%p", ssl);

         ssl   = 0;
         reuse = false;

         goto retry;
         }

      pcNewConnection->USocket::closesocket();

      pcNewConnection->iState    = -errno;
      pcNewConnection->iSockDesc = -1;

      U_INTERNAL_DUMP("pcNewConnection->ret = %d", pcNewConnection->ret)
      }

end:
   U_RETURN_POINTER(pcNewConnection, USocket);
}

USocket* USSLSocket::acceptClient(USocket* pcNewConnection)
{
   U_TRACE(0, "USSLSocket::acceptClient(%p)", pcNewConnection)

   if (UTCPSocket::accept(pcNewConnection) && active) (void) accept((USSLSocket*)pcNewConnection);

   U_RETURN_POINTER(pcNewConnection, USocket);
}

USocket* USSLSocket::acceptClient()
{
   U_TRACE(0, "USSLSocket::acceptClient()")

   USSLSocket* pcNewConnection = U_NEW(USSLSocket(bIPv6Socket, ctx));

   pcNewConnection->active = active;

   if (UTCPSocket::accept(pcNewConnection) && active) (void) accept(pcNewConnection);

   U_RETURN_POINTER(pcNewConnection, USocket);
}

int USSLSocket::recv(void* pBuffer, int iBufferLen)
{
   U_TRACE(1, "USSLSocket::recv(%p,%d)", pBuffer, iBufferLen)

   U_INTERNAL_ASSERT_EQUALS(USocket::isConnected(), true)

   int iBytesRead;

   if (active == false) iBytesRead = USocket::recv(pBuffer, iBufferLen);
   else
      {
      U_INTERNAL_ASSERT_POINTER(ssl)
loop:
      errno      = 0;
      iBytesRead = U_SYSCALL(SSL_read, "%p,%p,%d", ssl, CAST(pBuffer), iBufferLen);

      if (iBytesRead <= 0)
         {
         if (errno) iState = -errno;
         else
            {
            ret = U_SYSCALL(SSL_get_error, "%p,%d", ssl, ret);

            U_DUMP("status = %.*S", 512, status(false))

            if (ret == SSL_ERROR_WANT_READ) goto loop;
            }
         }
      }

#ifdef DEBUG
   if (iBytesRead > 0) U_INTERNAL_DUMP("BytesRead(%d) = %#.*S", iBytesRead, iBytesRead, CAST(pBuffer))
#endif

   U_RETURN(iBytesRead);
}

int USSLSocket::send(const void* pData, int iDataLen)
{
   U_TRACE(1, "USSLSocket::send(%p,%d)", pData, iDataLen)

   U_INTERNAL_ASSERT_EQUALS(USocket::isConnected(), true)

   int iBytesWrite;

   if (active == false) iBytesWrite = USocket::send(pData, iDataLen);
   else
      {
      U_INTERNAL_ASSERT_POINTER(ssl)
loop:
      errno       = 0;
      iBytesWrite = U_SYSCALL(SSL_write, "%p,%p,%d", ssl, CAST(pData), iDataLen);

      if (iBytesWrite <= 0)
         {
         if (errno) iState = -errno;
         else
            {
            ret = U_SYSCALL(SSL_get_error, "%p,%d", ssl, ret);

            U_DUMP("status = %.*S", 512, status(false))

            if (ret == SSL_ERROR_WANT_WRITE) goto loop;
            }
         }
      }

#ifdef DEBUG
   if (iBytesWrite > 0) U_INTERNAL_DUMP("BytesWrite(%d) = %#.*S", iBytesWrite, iBytesWrite, CAST(pData))
#endif

   U_RETURN(iBytesWrite);
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* USSLSocket::dump(bool reset) const
{
   USocket::dump(false);

   *UObjectIO::os << '\n'
                  << "ret                           " << ret        << '\n'
                  << "ssl                           " << (void*)ssl << '\n'
                  << "ctx                           " << (void*)ctx << '\n'
                  << "active                        " << active;

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
