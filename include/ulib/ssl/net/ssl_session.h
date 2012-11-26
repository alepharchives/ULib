// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    ssl_session.h - ssl session utility
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_SSL_SESSION_H
#define ULIB_SSL_SESSION_H 1

#include <ulib/ssl/net/sslsocket.h>

class URDB;
class UServer_Base;

/**
 * SSL Session Information
 *
 * This class contains data about an SSL session
 */

class U_EXPORT USSLSession {
public:

   // converts SSL_SESSION object from/to ASN1 representation

   static SSL_SESSION* fromString(const UString& data);
   static UString        toString(SSL_SESSION* session);

   // SERVICES

   static void deleteSessionCache();
   static bool   initSessionCache(SSL_CTX* ctx, const UString& pathdb, uint32_t sz);

   static int                newSession(SSL* ssl, SSL_SESSION* sess);
   static void            removeSession(SSL_CTX* ctx, SSL_SESSION* sess);
   static SSL_SESSION* getCachedSession(SSL* ssl, unsigned char* id, int len, int* copy);

protected:
   static UStringRep* pkey;
   static URDB* db_ssl_session;
   
private:
   USSLSession(const USSLSession&)            {}
   USSLSession& operator=(const USSLSession&) { return *this; }

   friend class UServer_Base;
};

#endif
