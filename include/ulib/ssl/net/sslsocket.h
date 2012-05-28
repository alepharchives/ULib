// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    sslsocket.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_SSLSOCKET_H
#define ULIB_SSLSOCKET_H 1

#include <ulib/net/tcpsocket.h>
#include <ulib/utility/services.h>

#include <openssl/ssl.h>
#include <openssl/x509.h>

/**
This class implements TCP/IP sockets with the Secure Sockets Layer (SSL v2/v3) and
Transport Layer Security (TLS v1) protocols. The OpenSSL library is used in this implementation,
see the OpenSSL homepage for more information.
--------------------------------------------------------------------------------------------------------------------
Quando un'applicazione (cliente) chiede di aprire una connessione SSL verso un server si attiva il servizio
che utilizza i protocolli di handshake e di scambio delle chiavi di cifratura. Lo scopo di questa fase è quello
di utilizzare un sistema crittografico asimmetrico per svolgere due funzioni:
1) Effettuare l'autenticazione reciproca del cliente e del server.
2) Scambiare fra cliente e server le chiavi di cifratura da utilizzare nel corso della connessione.
   Le chiavi sono due per il cliente e due per il server. Una di esse serve per la cifratura dei dati
   e l'altra per il codice di autenticazione (MAC).

Al termine di questa fase inizia la vera e propria connessione durante la quale il messaggio (ovvero i dati
inviati da cliente a server e viceversa) viene trattato come segue:
a) Il messaggio viene suddiviso in blocchi di lunghezza prefissata.
b) Ciascun blocco viene compresso con un algoritmo di compressione.
c) A ciascun blocco viene aggiunto il codice MAC che serve a garantire l'autenticità dei dati.
d) Il blocco complessivo (dati compressi+MAC) viene cifrato utilizzando un algoritmo simmetrico e l'opportuna chiave.
e) In testa al blocco cifrato viene posto lo "header" SSL.
f) Il blocco viene inviato al corrispondente utilizzando il protocollo TCP/IP.

Il corrispondente alla ricezione del blocco dovrà efettuare le operazioni contrarie, ovvero:
a) Eliminare lo header SSL
b) Decifrare il messaggio utilizzando la chiave opportuna
c) Verificare che il codice MAC sia quello del corrispondente
d) Eliminare il codice MAC
e) Decomprimere il blocco risultante
f) Riunire i frammenti del messaggio
---------------------------------------------------------------------------------------------------------------------
*/

#define SSL_VERIFY_PEER_STRICT (SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT)

typedef int (*PEM_PASSWD_CB)(char* buf, int size, int rwflag, void* password); // Password callback

class UServer_Base;
class UClientImage_Base;
template <class T> class UServer;
template <class T> class UClientImage;

class U_EXPORT USSLSocket : public UTCPSocket {
public:

   // COSTRUTTORI

    USSLSocket(bool bSocketIsIPv6 = false, SSL_CTX* ctx = 0, bool server = true);
   ~USSLSocket();

   // VARIE

   void setActive(bool flag)
      {
      U_TRACE(0, "USSLSocket::setActive(%b)", flag)

      active = flag;
      }

   bool secureConnection(int fd);
   bool acceptSSL(USSLSocket* pcConnection);

   static long getOptions(const UVector<UString>& vec);

   /**
   Load Diffie-Hellman parameters from file. These are used to generate a DH key exchange.
   See man SSL_CTX_set_tmp_dh_callback(3) and www.skip-vpn.org/spec/numbers.html for more information.
   Should be called before accept() or connect() if used. Returns true on success
   */

   bool useDHFile(const char* dh_file = 0);

   /**
   Load a certificate. A socket used on the server side needs to have a certificate (but a temporary RSA session
   certificate may be created if you don't load one yourself). The client side can also load certificates but it
   is not required. The files should be in ASCII PEM format and the certificate and the private key can either be
   in the same file or two separate files. OpenSSL's standard password prompt will be used if the private key uses
   a password. You should load the certificate before calling accept() or connect(). Should the peer certificate be
   verified ? The arguments specify the locations of trusted CA certificates used in the verification. Either CAfile
   or CApath can be set to NULL. See man SSL_CTX_load_verify_locations(3) for format information. Should be called
   before accept() or connect() if used and the verification result is then available by calling getVerifyResult()
   on the connected socket (the new socket from accept() on the server side, the same socket on the client side).
   Returns true on success
   */

   bool setContext(const char* dh_file, const char* cert_file,
                   const char* private_key_file, const char* passwd,
                   const char* CAfile, const char* CApath,
                   int mode = SSL_VERIFY_PEER_STRICT | SSL_VERIFY_CLIENT_ONCE);

   /**
   For successful verification, the peer certificate must be signed with the CA certificate directly or indirectly
   (a proper certificate chain exists). The certificate chain length from the CA certificate to the peer certificate
   can be set in the verify_depth field of the SSL_CTX and SSL structures. (The value in SSL is inherited from SSL_CTX
   when you create an SSL structure using the SSL_new() API). Setting verify_depth to 1 means that the peer certificate
   must be directly signed by the CA certificate. 
   */

   void setVerifyDepth(int depth = 1)
      {
      U_TRACE(1, "USSLSocket::setVerifyDepth(%d)", depth)

      U_INTERNAL_ASSERT_POINTER(ctx)

      U_SYSCALL_VOID(SSL_CTX_set_verify_depth, "%p,%d", ctx, depth);
      }

   /**
   Verify callback
   */

   void setVerifyCallback(verify_cb func, int mode = SSL_VERIFY_PEER_STRICT | SSL_VERIFY_CLIENT_ONCE)
      {
      U_TRACE(1, "USSLSocket::setVerifyCallback(%p,%d)", func, mode)

      U_INTERNAL_ASSERT_POINTER(func)

      U_SYSCALL_VOID(SSL_CTX_set_verify, "%p,%d,%p", ctx, mode, func);
      }

   /**
   Gets the peer certificate verification result. Should be called after connect() or accept() where the verification is
   done. On the server side this should be done on the new object returned by accept() and not on the listener object! If
   you don't get X509_V_OK and don't trust the peer you should disconnect. If you trust the peer (or perhaps ask the user
   if he/she does) but didn't get X509_V_OK you might consider adding this certificate to the trusted CA certificates loaded
   by setContext(), but don't add invalid certificates...
   */

   long getVerifyResult()
      {
      U_TRACE(1, "USSLSocket::getVerifyResult()")

      U_INTERNAL_ASSERT_POINTER(ssl)

      long result = U_SYSCALL(SSL_get_verify_result, "%p", ssl);

      U_DUMP("verify_status = %S", UServices::verify_status(result))

      U_RETURN(result);
      }

   /**
   Get peer certificate. Should be called after connect() or accept() when using verification
   NB: OpenSSL already tested the cert validity during SSL handshake and returns a X509 ptr just if the certificate is valid...
   */

   X509* getPeerCertificate()
      {
      U_TRACE(1, "USSLSocket::getPeerCertificate()")

      X509* peer = (X509*) (ssl ? U_SYSCALL(SSL_get_peer_certificate, "%p", ssl) : 0);

      U_RETURN_POINTER(peer, X509);
      }

   /**
   Server side RE-NEGOTIATE asking for client cert
   */

   bool askForClientCertificate();

   /**
   Returns the number of bytes which are available inside ssl for immediate read
   */

   uint32_t pending() const
      {
      U_TRACE(0, "USSLSocket::pending()")

      if (active == false) U_RETURN(0);

      U_INTERNAL_DUMP("this = %p ssl = %p", this, ssl)

      U_INTERNAL_ASSERT_POINTER(ssl)

      // NB: data are received in blocks from the peer. Therefore data can be buffered
      // inside ssl and are ready for immediate retrieval with SSL_read()...

      uint32_t result = U_SYSCALL(SSL_pending, "%p", ssl);

      U_RETURN(result);
      }

   // VIRTUAL METHOD

   virtual bool isSSL() const
      {
      U_TRACE(0, "USSLSocket::isSSL()")

      U_RETURN(true);
      }

   virtual void closesocket();

   virtual int send(const char* pData,   uint32_t iDataLen);
   virtual int recv(      void* pBuffer, uint32_t iBufferLen);

   virtual int writev(const struct iovec* iov, int iovcnt)
      {
      U_TRACE(0, "USSLSocket::writev(%p,%d)", iov, iovcnt)

      int result = USocket::_writev(iov, iovcnt);

      U_RETURN(result);
      }

   virtual const char* getMsgError(char* buffer, uint32_t buffer_size);

   /**
   This method is called to connect the socket to a server SSL that is specified
   by the provided host name and port number. We call the SSL_connect() function to perform the connection
   */

   virtual bool connectServer(const UString& server, int iServPort);

   /**
   This method is called to accept a new pending connection on the server socket.
   The USocket pointed to by the provided parameter is modified to refer to the
   newly connected socket. The remote IP Address and port number are also set.
   */

   virtual bool acceptClient(USocket* pcNewConnection)
      {
      U_TRACE(0, "USSLSocket::acceptClient(%p)", pcNewConnection)

      if (USocket::acceptClient(pcNewConnection) &&
          active                                 &&
          acceptSSL((USSLSocket*)pcNewConnection))
         {
         U_RETURN(true);
         }

      U_RETURN(false);
      }

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   SSL* ssl;
   SSL_CTX* ctx;
   int ret, renegotiations;
   bool active;

   static int session_cache_index;

   static void        info_callback(const SSL* ssl, int where, int ret);
   static const char* status(SSL* ssl, int ret, bool flag, char* buffer, uint32_t buffer_size);

          const char* status(bool flag) const { return status(ssl, ret, flag, 0, 0); }

   static SSL_CTX* getClientContext() { return getContext(0, false, 0); }
   static SSL_CTX* getServerContext() { return getContext(0, true,  0); }

   static SSL_CTX* getContext(SSL_METHOD* method, bool server, long options);

private:
   USSLSocket(const USSLSocket&) : UTCPSocket(false) {}
   USSLSocket& operator=(const USSLSocket&)          { return *this; }

                      friend class UServer_Base;
                      friend class UClientImage_Base;
   template <class T> friend class UServer;
   template <class T> friend class UClientImage;
};

#endif
