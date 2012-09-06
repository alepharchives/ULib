// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    ssl_session.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/db/rdb.h>
#include <ulib/net/server/server.h>
#include <ulib/ssl/net/ssl_session.h>

URDB*       USSLSession::db_ssl_session;
UStringRep* USSLSession::pkey;

/*
 * Forward secrecy
 *
 * You should consider forward secrecy. Forward secrecy means that the keys for a connection aren't stored on disk.
 * You might have limited the amount of information that you log in order to protect the privacy of your users, but
 * if you don't have forward secrecy then your private key is capable of decrypting all past connections. Someone else
 * might be doing the logging for you. In order to enable forward secrecy you need to have DHE or ECDHE ciphersuites as
 * your top preference. DHE ciphersuites are somewhat expensive if you're terminating lots of SSL connections and you
 * should be aware that your server will probably only allow 1024-bit DHE. I think 1024-bit DHE-RSA is preferable to
 * 2048-bit RSA, but opinions vary. If you're using ECDHE, use P-256. You also need to be aware of Session Tickets in
 * order to implement forward secrecy correctly. There are two ways to resume a TLS connection: either the server chooses
 * a random number and both sides store the session information, of the server can encrypt the session information with a
 * secret, local key and send that to the client. The former is called Session IDs and the latter is called Session Tickets.
 * But Session Tickets are transmitted over the wire and so the server's Session Ticket encryption key is capable of decrypting
 * past connections. Most servers will generate a random Session Ticket key at startup unless otherwise configured, but you should check.
 */

void USSLSession::deleteSessionCache()
{
   U_TRACE(0, "USSLSession::deleteSessionCache()")

   U_INTERNAL_ASSERT_POINTER(pkey)
   U_INTERNAL_ASSERT_POINTER(db_ssl_session)

   delete pkey;

          db_ssl_session->close();
   delete db_ssl_session;
}

bool USSLSession::initSessionCache(SSL_CTX* ctx, const char* location, uint32_t sz)
{
   U_TRACE(0, "USSLSession::initSessionCache(%p,%S,%u)", ctx, location, sz)

   U_INTERNAL_ASSERT_EQUALS(db_ssl_session,0)

   UString pathdb(U_CAPACITY);

   pathdb.snprintf("%s%s", (location[0] == '/' ? "" : U_LIBEXECDIR "/"), location);

   db_ssl_session = U_NEW(URDB(pathdb, false));

   if (((URDB*)db_ssl_session)->open(sz, true, true) == false) // NB: we want truncate...
      {
      U_SRV_LOG("db initialization of SSL session failed...");

      delete (URDB*)db_ssl_session;
                    db_ssl_session = 0;

      U_RETURN(false);
      }

   pkey = UStringRep::create(0U, 100U, 0);

   /* In order to allow external session caching, synchronization with the internal session cache is realized via callback functions.
    * Inside these callback functions, session can be saved to disk or put into a database using the d2i_SSL_SESSION(3) interface.
    *
    * The new_session_cb() is called, whenever a new session has been negotiated and session caching is enabled
    * (see SSL_CTX_set_session_cache_mode(3)). The new_session_cb() is passed the ssl connection and the ssl session sess.
    * If the callback returns 0, the session will be immediately removed again.
    *
    * The remove_session_cb() is called, whenever the SSL engine removes a session from the internal cache. This happens when
    * the session is removed because it is expired or when a connection was not shutdown cleanly. It also happens for all sessions
    * in the internal session cache when SSL_CTX_free(3) is called. The remove_session_cb() is passed the ctx and the ssl session sess.
    * It does not provide any feedback.
    *
    * The get_session_cb() is only called on SSL/TLS servers with the session id proposed by the client. The get_session_cb() is
    * always called, also when session caching was disabled. The get_session_cb() is passed the ssl connection, the session id and
    * the length at the memory location data. With the parameter copy the callback can require the SSL engine to increment the
    * reference count of the SSL_SESSION object, Normally the reference count is not incremented and therefore the session must not
    * be explicitly freed with SSL_SESSION_free(3)
    */

   U_SYSCALL_VOID(SSL_CTX_sess_set_new_cb,    "%p,%p", ctx, newSession);
   U_SYSCALL_VOID(SSL_CTX_sess_set_get_cb,    "%p,%p", ctx, getCachedSession);
   U_SYSCALL_VOID(SSL_CTX_sess_set_remove_cb, "%p,%p", ctx, removeSession);

   // NB: All currently supported protocols have the same default timeout value of 300 seconds
   // ----------------------------------------------------------------------------------------
   // (void) U_SYSCALL(SSL_CTX_set_timeout,         "%p,%u", ctx, 300);
      (void) U_SYSCALL(SSL_CTX_sess_set_cache_size, "%p,%u", ctx, sz);

   U_INTERNAL_DUMP("timeout = %d", SSL_CTX_get_timeout(ctx))

   U_SRV_LOG("db initialization of SSL session %s success", location);

   U_RETURN(true);
}

// converts SSL_SESSION object from/to ASN1 representation

UString USSLSession::toString(SSL_SESSION* sess)
{
   U_TRACE(0, "USSLSession::toString(%p)", sess)

   U_INTERNAL_ASSERT_POINTER(sess)

   unsigned char buffer[256 * 1024];
   unsigned char* p = buffer;

   int slen = U_SYSCALL(i2d_SSL_SESSION, "%p,%p", sess, &p);

   if (slen <= 0) U_RETURN_STRING(UString::getStringNull());

   U_INTERNAL_ASSERT_MINOR(slen, (int)sizeof(buffer))

   UString x((void*)buffer, slen);

   U_RETURN_STRING(x);
}

SSL_SESSION* USSLSession::fromString(const UString& x)
{
   U_TRACE(0, "USSLSession::fromString(%.*S)", U_STRING_TO_TRACE(x))

   U_ASSERT_EQUALS(x.empty(), false)

#ifdef HAVE_OPENSSL_97
         unsigned char* p =       (unsigned char*)x.data();
#else
   const unsigned char* p = (const unsigned char*)x.data();
#endif

   SSL_SESSION* sess = (SSL_SESSION*) U_SYSCALL(d2i_SSL_SESSION, "%p,%p,%ld", 0, &p, (long)x.size());

   U_RETURN_POINTER(sess,SSL_SESSION);
}

int USSLSession::newSession(SSL* ssl, SSL_SESSION* sess)
{
   U_TRACE(0, "USSLSession::newSession(%p,%p)", ssl, sess)

/*
#ifdef DEBUG
   static FILE* fp = (FILE*) U_SYSCALL(fopen, "%S,%S", "/tmp/ssl_session.new", "a");

   if (fp) (void) U_SYSCALL(SSL_SESSION_print_fp, "%p,%p", fp, sess);
#endif
*/

   UString value = toString(sess);

   U_ASSERT_EQUALS(value.empty(), false)

   if (value.size() <= 4096) // do not cache too big session
      {
      pkey->str     = (const char*)sess->session_id;
      pkey->_length =              sess->session_id_length;

      U_INTERNAL_DUMP("pkey(%u) = %#.*S", pkey->size(), pkey->size(), pkey->data())

      int result = db_ssl_session->store(pkey, value, RDB_REPLACE);

      if (result) U_SRV_LOG("store of SSL session on db failed with error %d", result);
      }

   U_RETURN(0);
}

SSL_SESSION* USSLSession::getCachedSession(SSL* ssl, unsigned char* id, int len, int* copy)
{
   U_TRACE(0, "USSLSession::getCachedSession(%p,%.*S,%d,%p)", ssl, len, id, len, copy)

   *copy = 0;

   pkey->str     = (const char*)id;
   pkey->_length = len;

   U_INTERNAL_DUMP("pkey(%u) = %#.*S", pkey->size(), pkey->size(), pkey->data())

   UString value = (*db_ssl_session)[pkey];

   if (value.empty()) U_RETURN_POINTER(0,SSL_SESSION);

   SSL_SESSION* sess = fromString(value);

/*
#ifdef DEBUG
   static FILE* fp = (FILE*) U_SYSCALL(fopen, "%S,%S", "/tmp/ssl_session.get", "a");

   if (fp) (void) U_SYSCALL(SSL_SESSION_print_fp, "%p,%p", fp, sess);
#endif
*/

   U_RETURN_POINTER(sess,SSL_SESSION);
}

void USSLSession::removeSession(SSL_CTX* ctx, SSL_SESSION* sess)
{
   U_TRACE(0, "USSLSession::removeSession(%p,%p)", ctx, sess)

/*
#ifdef DEBUG
   static FILE* fp = (FILE*) U_SYSCALL(fopen, "%S,%S", "/tmp/ssl_session.del", "a");

   if (fp) (void) U_SYSCALL(SSL_SESSION_print_fp, "%p,%p", fp, sess);
#endif
*/

   UString key((const char*)sess->session_id, (uint32_t)sess->session_id_length);

   U_INTERNAL_DUMP("key(%u) = %#.*S", key.size(), key.size(), key.data())

   int result = db_ssl_session->remove(key);

   // -2: The entry was already marked deleted in the hash-tree

   if (result && result != -2) U_SRV_LOG("remove of SSL session on db failed with error %d", result);
}
