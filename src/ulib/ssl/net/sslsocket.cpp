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

#include <ulib/notifier.h>
#include <ulib/utility/services.h>
#include <ulib/ssl/net/sslsocket.h>
#include <ulib/utility/socket_ext.h>

#include <openssl/err.h>

#ifndef SSL_ERROR_WANT_ACCEPT
#define SSL_ERROR_WANT_ACCEPT SSL_ERROR_WANT_READ
#endif

int USSLSocket::session_cache_index;

/* The OpenSSL ssl library implements the Secure Sockets Layer (SSL v2/v3) and Transport Layer Security (TLS v1) protocols.
 * It provides a rich API. At first the library must be initialized; see SSL_library_init(3). Then an SSL_CTX object is created
 * as a framework to establish TLS/SSL enabled connections (see SSL_CTX_new(3)). Various options regarding certificates, algorithms
 * etc. can be set in this object. When a network connection has been created, it can be assigned to an SSL object. After the SSL
 * object has been created using SSL_new(3), SSL_set_fd(3) or SSL_set_bio(3) can be used to associate the network connection with
 * the object. Then the TLS/SSL handshake is performed using SSL_accept(3) or SSL_connect(3) respectively. SSL_read(3) and SSL_write(3)
 * are used to read and write data on the TLS/SSL connection. SSL_shutdown(3) can be used to shut down the TLS/SSL connection. 
 *
 * When packets in SSL arrive at a destination, they are pulled off the socket in chunks of sizes controlled by the encryption protocol being
 * used, decrypted, and placed in SSL-internal buffers. The buffer content is then transferred to the application program through SSL_read().
 * If you've read only part of the decrypted data, there will still be pending input data on the SSL connection, but it won't show up on the
 * underlying file descriptor via select(). Your code needs to call SSL_pending() explicitly to see if there is any pending data to be read.
 *
 * NON-blocking I/O
 *
 * A pitfall to avoid: Don't assume that SSL_read() will just read from the underlying transport or that SSL_write() will just write to it
 * it is also possible that SSL_write() cannot do any useful work until there is data to read, or that SSL_read() cannot do anything until
 * it is possible to send data. One reason for this is that the peer may request a new TLS/SSL handshake at any time during the protocol,
 * requiring a bi-directional message exchange; both SSL_read() and SSL_write() will try to continue any pending handshake.
 */

USSLSocket::USSLSocket(bool bSocketIsIPv6, SSL_CTX* _ctx, bool server) : UTCPSocket(bSocketIsIPv6)
{
   U_TRACE_REGISTER_OBJECT(0, USSLSocket, "%b,%p,%b", bSocketIsIPv6, _ctx, server)

   if (_ctx == 0) ctx = (server ? getServerContext() : getClientContext());
   else
      {
      ctx = _ctx;

      ctx->references++; // We don't want our destructor to delete ctx if still in use...
      }

   ssl    = 0;
   ret    = renegotiations = 0;
   active = true;
}

USSLSocket::~USSLSocket()
{
   U_TRACE_UNREGISTER_OBJECT(0, USSLSocket)

   U_INTERNAL_ASSERT_POINTER(ctx)

   if (ssl) U_SYSCALL_VOID(SSL_free,     "%p", ssl); /* SSL_free will free UServices::store */
            U_SYSCALL_VOID(SSL_CTX_free, "%p", ctx);
}

void USSLSocket::info_callback(const SSL* ssl, int where, int ret)
{
   U_TRACE(0, "USSLSocket::info_callback(%p,%d,%d)", ssl, where, ret)

   if ((where & SSL_CB_HANDSHAKE_START) != 0)
      {
      U_INTERNAL_DUMP("SSL_CB_HANDSHAKE_START")

      USSLSocket* pobj = (USSLSocket*) SSL_get_app_data((SSL*)ssl);

      if (pobj)
         {
         pobj->renegotiations++;

         U_INTERNAL_DUMP("pobj->renegotiations = %d", pobj->renegotiations)
         }
      }
   else if ((where & SSL_CB_HANDSHAKE_DONE) != 0)
      {
      U_INTERNAL_DUMP("SSL_CB_HANDSHAKE_DONE")

      ssl->s3->flags |= SSL3_FLAGS_NO_RENEGOTIATE_CIPHERS;
      }
}

SSL_CTX* USSLSocket::getContext(SSL_METHOD* method, bool server, long options)
{
   U_TRACE(0, "USSLSocket::getContext(%p,%b,%ld)", method, server, options)

   if (method == 0)
      {
      if (server)
         {
#     if !defined(OPENSSL_NO_TLS1) && defined(TLS1_2_VERSION)
         method = (SSL_METHOD*)TLSv1_2_server_method();
#     elif !defined(OPENSSL_NO_SSL3)
         method = (SSL_METHOD*)SSLv23_server_method();
#     elif !defined(OPENSSL_NO_SSL2)
         method = (SSL_METHOD*)SSLv2_server_method();
#     endif
         }
      else
         {
#     if !defined(OPENSSL_NO_TLS1) && defined(TLS1_2_VERSION)
         method = (SSL_METHOD*)TLSv1_2_client_method();
#     elif !defined(OPENSSL_NO_SSL3)
         method = (SSL_METHOD*)SSLv3_client_method();
#     elif !defined(OPENSSL_NO_SSL2)
         method = (SSL_METHOD*)SSLv2_client_method();
#     endif
         }
      }

   SSL_CTX* ctx = (SSL_CTX*) U_SYSCALL(SSL_CTX_new, "%p", method);

   U_SYSCALL_VOID(SSL_CTX_set_quiet_shutdown,     "%p,%d", ctx, 1);
   U_SYSCALL_VOID(SSL_CTX_set_default_read_ahead, "%p,%d", ctx, 1);

   (void) U_SYSCALL(SSL_CTX_set_options, "%p,%d", ctx, (options ? options 
                                                                : SSL_OP_NO_SSLv2        |
#                                                              ifdef SSL_OP_NO_COMPRESSION
                                                                  SSL_OP_NO_COMPRESSION |
#                                                              endif
                                                                  SSL_OP_CIPHER_SERVER_PREFERENCE));

   if (server)
      {
      U_INTERNAL_ASSERT_MINOR(u_progname_len, SSL_MAX_SSL_SESSION_ID_LENGTH)

      (void) U_SYSCALL(SSL_CTX_set_session_cache_mode, "%p,%d",    ctx, SSL_SESS_CACHE_SERVER);
      (void) U_SYSCALL(SSL_CTX_set_session_id_context, "%p,%p,%u", ctx, (const unsigned char*)u_progname, u_progname_len);

      // We need this to disable client-initiated renegotiation

      U_SYSCALL_VOID(SSL_CTX_set_info_callback, "%p,%p", ctx, USSLSocket::info_callback);
      }

   // Disable support for low encryption ciphers

   (void) U_SYSCALL(SSL_CTX_set_cipher_list, "%p,%S", ctx, "ECDHE-RSA-AES256-SHA384:AES256-SHA256:RC4-SHA:RC4:HIGH:!MD5:!aNULL:!EDH:!AESGCM");

// (void) U_SYSCALL(SSL_CTX_set_mode, "%p,%d", ctx, SSL_CTX_get_mode(ctx) | SSL_MODE_AUTO_RETRY);

   U_RETURN_POINTER(ctx,SSL_CTX);
}

/* get OpenSSL-specific options (default: NO_SSLv2, CIPHER_SERVER_PREFERENCE, NO_COMPRESSION)
 * to overwrite defaults you need to explicitly specify the reverse flag (toggle "NO_" prefix)
 *
 * example: use sslv2 and compression: [ options: ("SSLv2", "COMPRESSION") ]
 */

long USSLSocket::getOptions(const UVector<UString>& vec)
{
   U_TRACE(0, "USSLSocket::getOptions(%p", &vec)

   static const struct {
      const char*   name;      // without "NO_" prefix
      uint32_t      name_len;
      unsigned long value;
      char          positive;  // 0 means option is usually prefixed with "NO_"; otherwise use 1
   } option_table[] = {
   {U_CONSTANT_TO_PARAM("MICROSOFT_SESS_ID_BUG"), SSL_OP_MICROSOFT_SESS_ID_BUG, 1},
   {U_CONSTANT_TO_PARAM("NETSCAPE_CHALLENGE_BUG"), SSL_OP_NETSCAPE_CHALLENGE_BUG, 1},
#ifdef SSL_OP_LEGACY_SERVER_CONNECT
   {U_CONSTANT_TO_PARAM("LEGACY_SERVER_CONNECT"), SSL_OP_LEGACY_SERVER_CONNECT, 1},
#endif
   {U_CONSTANT_TO_PARAM("NETSCAPE_REUSE_CIPHER_CHANGE_BUG"), SSL_OP_NETSCAPE_REUSE_CIPHER_CHANGE_BUG, 1},
   {U_CONSTANT_TO_PARAM("SSLREF2_REUSE_CERT_TYPE_BUG"), SSL_OP_SSLREF2_REUSE_CERT_TYPE_BUG, 1},
   {U_CONSTANT_TO_PARAM("MICROSOFT_BIG_SSLV3_BUFFER"), SSL_OP_MICROSOFT_BIG_SSLV3_BUFFER, 1},
   {U_CONSTANT_TO_PARAM("MSIE_SSLV2_RSA_PADDING"), SSL_OP_MSIE_SSLV2_RSA_PADDING, 1},
   {U_CONSTANT_TO_PARAM("SSLEAY_080_CLIENT_DH_BUG"), SSL_OP_SSLEAY_080_CLIENT_DH_BUG, 1},
   {U_CONSTANT_TO_PARAM("TLS_D5_BUG"), SSL_OP_TLS_D5_BUG, 1},
   {U_CONSTANT_TO_PARAM("TLS_BLOCK_PADDING_BUG"), SSL_OP_TLS_BLOCK_PADDING_BUG, 1},
#ifdef SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS
   {U_CONSTANT_TO_PARAM("DONT_INSERT_EMPTY_FRAGMENTS"), SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS, 1},
#endif
   {U_CONSTANT_TO_PARAM("ALL"), SSL_OP_ALL, 1},
#ifdef SSL_OP_NO_QUERY_MTU
   {U_CONSTANT_TO_PARAM("QUERY_MTU"), SSL_OP_NO_QUERY_MTU, 0},
#endif
#ifdef SSL_OP_COOKIE_EXCHANGE
   {U_CONSTANT_TO_PARAM("COOKIE_EXCHANGE"), SSL_OP_COOKIE_EXCHANGE, 1},
#endif
#ifdef SSL_OP_NO_TICKET
   {U_CONSTANT_TO_PARAM("TICKET"), SSL_OP_NO_TICKET, 0},
#endif
#ifdef SSL_OP_CISCO_ANYCONNECT
   {U_CONSTANT_TO_PARAM("CISCO_ANYCONNECT"), SSL_OP_CISCO_ANYCONNECT, 1},
#endif
#ifdef SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION
   {U_CONSTANT_TO_PARAM("SESSION_RESUMPTION_ON_RENEGOTIATION"), SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION, 0},
#endif
#ifdef SSL_OP_NO_COMPRESSION
   {U_CONSTANT_TO_PARAM("COMPRESSION"), SSL_OP_NO_COMPRESSION, 0},
#endif
#ifdef SSL_OP_ALLOW_UNSAFE_LEGACY_RENEGOTIATION
   {U_CONSTANT_TO_PARAM("ALLOW_UNSAFE_LEGACY_RENEGOTIATION"), SSL_OP_ALLOW_UNSAFE_LEGACY_RENEGOTIATION, 1},
#endif
#ifdef SSL_OP_SINGLE_ECDH_USE
   {U_CONSTANT_TO_PARAM("SINGLE_ECDH_USE"), SSL_OP_SINGLE_ECDH_USE, 1},
#endif
   {U_CONSTANT_TO_PARAM("SINGLE_DH_USE"), SSL_OP_SINGLE_DH_USE, 1},
   {U_CONSTANT_TO_PARAM("EPHEMERAL_RSA"), SSL_OP_EPHEMERAL_RSA, 1},
#ifdef SSL_OP_CIPHER_SERVER_PREFERENCE
   {U_CONSTANT_TO_PARAM("CIPHER_SERVER_PREFERENCE"), SSL_OP_CIPHER_SERVER_PREFERENCE, 1},
#endif
   {U_CONSTANT_TO_PARAM("TLS_ROLLBACK_BUG"), SSL_OP_TLS_ROLLBACK_BUG, 1},
   {U_CONSTANT_TO_PARAM("SSLv2"), SSL_OP_NO_SSLv2, 0},
   {U_CONSTANT_TO_PARAM("SSLv3"), SSL_OP_NO_SSLv3, 0},
   {U_CONSTANT_TO_PARAM("TLSv1"), SSL_OP_NO_TLSv1, 0},
   {U_CONSTANT_TO_PARAM("PKCS1_CHECK_1"), SSL_OP_PKCS1_CHECK_1, 1},
   {U_CONSTANT_TO_PARAM("PKCS1_CHECK_2"), SSL_OP_PKCS1_CHECK_2, 1},
   {U_CONSTANT_TO_PARAM("NETSCAPE_CA_DN_BUG"), SSL_OP_NETSCAPE_CA_DN_BUG, 1},
   {U_CONSTANT_TO_PARAM("NETSCAPE_DEMO_CIPHER_CHANGE_BUG"), SSL_OP_NETSCAPE_DEMO_CIPHER_CHANGE_BUG, 1},
#ifdef SSL_OP_CRYPTOPRO_TLSEXT_BUG
   {U_CONSTANT_TO_PARAM("CRYPTOPRO_TLSEXT_BUG"), SSL_OP_CRYPTOPRO_TLSEXT_BUG, 1}
#endif
   };

   UString key;
   char positive;
   const char* ptr;
   uint32_t j, len;

   long options = SSL_OP_NO_SSLv2 |
                  SSL_OP_CIPHER_SERVER_PREFERENCE
#ifdef SSL_OP_NO_COMPRESSION
                | SSL_OP_NO_COMPRESSION
#endif
   ;

   for (uint32_t i = 0, n = vec.size(); i < n; ++i)
      {
      len      = key.size();
      ptr      = key.data();
      positive = 1;

      if (U_STRNCASECMP(ptr, "NO_") == 0)
         {
         ptr += 3;
         len -= 3;

         positive = 0;
         }

      for (j = 0; j < sizeof(option_table)/sizeof(option_table[0]); ++j)
         {
         if (option_table[j].name_len == len &&
             strcasecmp(ptr, option_table[j].name) == 0)
            {
            if (option_table[j].positive == positive) options |=  option_table[j].value;
            else                                      options &= ~option_table[j].value;
            }
         }
      }

   U_RETURN(options);
}

bool USSLSocket::useDHFile(const char* dh_file)
{
   U_TRACE(1, "USSLSocket::useDHFile(%S)", dh_file)

   // Set up DH stuff

   DH* dh;

   if ( dh_file &&
       *dh_file)
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

bool USSLSocket::setContext(const char* dh_file, const char* cert_file, const char* private_key_file,
                            const char* passwd,  const char* CAfile,    const char* CApath, int verify_mode)
{
   U_TRACE(1, "USSLSocket::setContext(%S,%S,%S,%S,%S,%S,%d)", dh_file, cert_file, private_key_file, passwd, CAfile, CApath, verify_mode)

   U_INTERNAL_ASSERT_POINTER(ctx)

   // These are the 1024 bit DH parameters from "Assigned Number for SKIP Protocols"
   // (http://www.skip-vpn.org/spec/numbers.html).
   // See there for how they were generated.

   if (useDHFile(dh_file) == false) U_RETURN(false);

   int result;

   // Load CERT PEM file

   if ( cert_file &&
       *cert_file)
      {
      result = U_SYSCALL(SSL_CTX_use_certificate_chain_file, "%p,%S", ctx, cert_file);

      if (result == 0) U_RETURN(false);
      }

   // Load private key PEM file and give passwd callback if any

   if ( private_key_file &&
       *private_key_file)
      {
      if ( passwd &&
          *passwd)
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
      if (UServices::setupOpenSSLStore(CAfile, CApath, (verify_mode ? U_STORE_FLAGS : 0)) == false) U_RETURN(false);

      U_SYSCALL_VOID(SSL_CTX_set_cert_store, "%p,%p", ctx, UServices::store);

      /* Sets the list of CAs sent to the client when requesting a client certificate for ctx */

      if (CAfile) // Process CA certificate bundle file
         {
         STACK_OF(X509_NAME)* list = (STACK_OF(X509_NAME)*) U_SYSCALL(SSL_load_client_CA_file, "%S", CAfile);

         U_SYSCALL_VOID(SSL_CTX_set_client_CA_list, "%p,%p", ctx, list);
         }
      }

   setVerifyCallback(UServices::X509Callback, verify_mode);

   U_RETURN(true);
}

const char* USSLSocket::status(SSL* _ssl, int _ret, bool flag, char* buffer, uint32_t buffer_size)
{
   U_TRACE(1, "USSLSocket::status(%p,%d,%b,%p,%u)", _ssl, _ret, flag, buffer, buffer_size)

   if (buffer == 0)
      {
      static char lbuffer[4096];

      buffer      = lbuffer;
      buffer_size = 4096;
      }

   const char* descr  = "SSL_ERROR_NONE";
   const char* errstr = "ok";

   if (_ret != SSL_ERROR_NONE) // 0
      {
      if (flag) _ret = U_SYSCALL(SSL_get_error, "%p,%d", _ssl, _ret);

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

      switch (_ret)
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

#     if defined(HAVE_OPENSSL_97) || defined(HAVE_OPENSSL_98)
         case SSL_ERROR_WANT_ACCEPT:
            {
            descr  = "SSL_ERROR_WANT_ACCEPT";
            errstr = "SSL Accept operation didn't complete, the same function should be called again later";
            }
         break;
#     endif
         }
      }

   uint32_t len, buffer_len = u__snprintf(buffer, buffer_size, "(%d, %s) - %s", _ret, descr, errstr);

   char* sslerr = UServices::getOpenSSLError(0, 0, &len);

   if (len)
      {
      buffer[buffer_len++] = ' ';

      U__MEMCPY((void*)(buffer + buffer_len), sslerr, len+1);
      }

   U_RETURN((const char*)buffer);
}

const char* USSLSocket::getMsgError(char* buffer, uint32_t buffer_size)
{
   U_TRACE(0, "USSLSocket::getMsgError(%p,%u)", buffer, buffer_size)

   U_INTERNAL_DUMP("ret = %d", ret)

   buffer = (char*) (ret == SSL_ERROR_NONE ? USocket::getMsgError(buffer, buffer_size)
                                           : status(ssl, ret, false, buffer, buffer_size));

   U_RETURN((const char*)buffer);
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

#     ifdef __MINGW32__
         USocket::fh = (SOCKET)_get_osfhandle(fd) ;
#     endif

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

   The only difference between the calls is that SSL_CTX_set_verify() sets the verification mode
   for all SSL objects derived from a given SSL_CTX —as long as they are created after SSL_CTX_set_verify()
   is called—whereas SSL_set_verify() only affects the SSL object that it is called on
   */

   U_SYSCALL_VOID(SSL_set_verify, "%p,%d,%p", ssl, SSL_VERIFY_PEER_STRICT, 0); // | SSL_VERIFY_CLIENT_ONCE

   /* Stop the client from just resuming the un-authenticated session */

   (void) U_SYSCALL(SSL_set_session_id_context, "%p,%p,%u", ssl, (const unsigned char*)this, sizeof(void*));

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

bool USSLSocket::acceptSSL(USSLSocket* pcNewConnection)
{
   U_TRACE(1, "USSLSocket::acceptSSL(%p)", pcNewConnection)

   int fd         = pcNewConnection->iSockDesc;
   uint32_t count = 0;

   U_DUMP("fd = %d isBlocking() = %b", fd, pcNewConnection->isBlocking())

   U_INTERNAL_ASSERT_EQUALS(ssl, 0)

   ssl = (SSL*) U_SYSCALL(SSL_new, "%p", ctx);

   // --------------------------------------------------------------------------------------------------
   // When beginning a new handshake, the SSL engine must know whether it must call the connect (client)
   // or accept (server) routines. Even though it may be clear from the method chosen, whether client or
   // server mode was requested, the handshake routines must be explicitly set.
   // --------------------------------------------------------------------------------------------------
   // U_SYSCALL_VOID(SSL_set_accept_state, "%p", ssl); // init SSL server session
   // --------------------------------------------------------------------------------------------------

   (void) U_SYSCALL(SSL_set_fd, "%p,%d", ssl, fd); // get SSL to use our socket

loop:
   errno = 0;
   ret   = U_SYSCALL(SSL_accept, "%p", ssl); // get SSL handshake with client

   if (ret == 1)
      {
      SSL_set_app_data(ssl, pcNewConnection);

      pcNewConnection->ssl            = ssl;
      pcNewConnection->ret            = SSL_ERROR_NONE;
      pcNewConnection->iState         = CONNECT;
      pcNewConnection->renegotiations = 0;

      ssl = 0;

      U_RETURN(true);
      }

   U_INTERNAL_DUMP("errno = %d", errno)

   if (errno) pcNewConnection->iState = -errno;

   pcNewConnection->ret = U_SYSCALL(SSL_get_error, "%p,%d", ssl, ret);

   U_DUMP("status = %.*S", 512, status(ssl, pcNewConnection->ret, false, 0, 0))

   U_INTERNAL_DUMP("count = %u", count)

   if (count++ < 5)
      {
           if (pcNewConnection->ret == SSL_ERROR_WANT_READ)  { if (UNotifier::waitForRead( fd, U_SSL_TIMEOUT_MS) > 0) goto loop; }
      else if (pcNewConnection->ret == SSL_ERROR_WANT_WRITE) { if (UNotifier::waitForWrite(fd, U_SSL_TIMEOUT_MS) > 0) goto loop; }
      }

   errno = -pcNewConnection->iState;

   U_SYSCALL_VOID(SSL_free, "%p", ssl);
                                  ssl = 0;

   pcNewConnection->USocket::_closesocket();

   U_INTERNAL_DUMP("pcNewConnection->ret = %d", pcNewConnection->ret)

   U_RETURN(false);
}

// VIRTUAL METHOD

#ifdef closesocket
#undef closesocket
#endif

void USSLSocket::closesocket()
{
   U_TRACE(1, "USSLSocket::closesocket()")

   if (ssl)
      {
      U_INTERNAL_DUMP("isTimeout() = %b", USocket::isTimeout())

      int mode = SSL_SENT_SHUTDOWN     |
                 SSL_RECEIVED_SHUTDOWN |
                 (USocket::isTimeout() ? 0
                                       : SSL_get_shutdown(ssl));

      U_SYSCALL_VOID(SSL_set_shutdown,       "%p,%d", ssl, mode);
      U_SYSCALL_VOID(SSL_set_quiet_shutdown, "%p,%d", ssl, 1);

loop:
      ret = U_SYSCALL(SSL_shutdown, "%p", ssl); // Send SSL shutdown signal to peer

      /* SSL_shutdown() never returns -1, on error it returns 0 */

      if (ret == 0)
         {
         ret = U_SYSCALL(SSL_get_error, "%p,%d", ssl, ret);

         U_DUMP("status = %.*S", 512, status(ssl, ret, false, 0, 0))

              if (ret == SSL_ERROR_WANT_READ)  { if (UNotifier::waitForRead( USocket::iSockDesc, U_SSL_TIMEOUT_MS) > 0) goto loop; }
         else if (ret == SSL_ERROR_WANT_WRITE) { if (UNotifier::waitForWrite(USocket::iSockDesc, U_SSL_TIMEOUT_MS) > 0) goto loop; }
         }

      U_SYSCALL_VOID(SSL_free, "%p", ssl);
                                     ssl = 0;                
      }

   if (USocket::isOpen()) UTCPSocket::closesocket();
}

bool USSLSocket::connectServer(const UString& server, int iServPort)
{
   U_TRACE(0, "USSLSocket::connectServer(%.*S,%d)", U_STRING_TO_TRACE(server), iServPort)

   if (UTCPSocket::connectServer(server, iServPort) &&
       (active ? secureConnection(USocket::iSockDesc) : true))
      {
      U_RETURN(true);
      }

   U_RETURN(false);
}

int USSLSocket::recv(void* pBuffer, uint32_t iBufferLen)
{
   U_TRACE(1, "USSLSocket::recv(%p,%u)", pBuffer, iBufferLen)

   U_INTERNAL_ASSERT(USocket::isConnected())

   int iBytesRead, lerrno;

   if (active == false)
      {
      iBytesRead = USocket::recv(pBuffer, iBufferLen);

      goto end;
      }

   U_INTERNAL_ASSERT_POINTER(ssl)

   lerrno     = 0;
loop:
   errno      = 0;
   iBytesRead = U_SYSCALL(SSL_read, "%p,%p,%d", ssl, CAST(pBuffer), iBufferLen);

   U_INTERNAL_DUMP("renegotiations = %d", renegotiations)

   if (renegotiations > 1)
      {
      U_WARNING("SSL: renegotiation initiated by client");

      while (ERR_peek_error())
         {
         U_WARNING("SSL: ignoring stale global SSL error");
         }

      ERR_clear_error();

      U_RETURN(-1);
      }

   if (iBytesRead > 0)
      {
      U_INTERNAL_DUMP("BytesRead(%d) = %#.*S", iBytesRead, iBytesRead, CAST(pBuffer))

      goto end;
      }

   U_INTERNAL_DUMP("errno = %d", errno)

   if (errno) lerrno = errno;

   ret = U_SYSCALL(SSL_get_error, "%p,%d", ssl, ret);

   U_DUMP("status = %.*S", 512, status(ssl, ret, false, 0, 0))

        if (ret == SSL_ERROR_WANT_READ)  { if (UNotifier::waitForRead( USocket::iSockDesc, U_SSL_TIMEOUT_MS) > 0) goto loop; }
   else if (ret == SSL_ERROR_WANT_WRITE) { if (UNotifier::waitForWrite(USocket::iSockDesc, U_SSL_TIMEOUT_MS) > 0) goto loop; }

   errno = lerrno;

end:
   U_RETURN(iBytesRead);
}

int USSLSocket::send(const char* pData, uint32_t iDataLen)
{
   U_TRACE(1, "USSLSocket::send(%p,%u)", pData, iDataLen)

   U_INTERNAL_ASSERT(USocket::isOpen())

   int iBytesWrite, lerrno;

   if (active == false)
      {
      iBytesWrite = USocket::send(pData, iDataLen);

      goto end;
      }

   U_INTERNAL_ASSERT_POINTER(ssl)

   lerrno      = 0;
loop:
   errno       = 0;
   iBytesWrite = U_SYSCALL(SSL_write, "%p,%p,%d", ssl, CAST(pData), iDataLen);

   if (iBytesWrite > 0)
      {
      U_INTERNAL_DUMP("BytesWrite(%d) = %#.*S", iBytesWrite, iBytesWrite, CAST(pData))

      goto end;
      }

   U_INTERNAL_DUMP("errno = %d", errno)

   if (errno) lerrno = errno;

   ret = U_SYSCALL(SSL_get_error, "%p,%d", ssl, ret);

   U_DUMP("status = %.*S", 512, status(ssl, ret, false, 0, 0))

        if (ret == SSL_ERROR_WANT_READ)  { if (UNotifier::waitForRead( USocket::iSockDesc, U_SSL_TIMEOUT_MS) > 0) goto loop; }
   else if (ret == SSL_ERROR_WANT_WRITE) { if (UNotifier::waitForWrite(USocket::iSockDesc, U_SSL_TIMEOUT_MS) > 0) goto loop; }

   errno = lerrno;

end:
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
                  << "active                        " << active     << '\n'
                  << "renegotiations                " << renegotiations;

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
