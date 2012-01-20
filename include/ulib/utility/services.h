// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    services.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_SERVICES_H
#define ULIB_SERVICES_H 1

#include <ulib/string.h>

#ifdef HAVE_SSL
#  include <openssl/pem.h>
#  include <openssl/engine.h>
#  include <openssl/x509_vfy.h>
typedef int (*verify_cb)(int,X509_STORE_CTX*); /* error callback */
#  ifndef X509_V_FLAG_CRL_CHECK
#  define X509_V_FLAG_CRL_CHECK     0x4
#  endif
#  ifndef X509_V_FLAG_CRL_CHECK_ALL
#  define X509_V_FLAG_CRL_CHECK_ALL 0x8
#  endif
#  define U_STORE_FLAGS (X509_V_FLAG_CRL_CHECK | X509_V_FLAG_CRL_CHECK_ALL)
#endif

#ifdef HAVE_LIBUUID
#  include <uuid/uuid.h>
#endif

#ifdef HAVE_FNMATCH
#  include <fnmatch.h>
#endif

#ifndef FNM_CASEFOLD
#define FNM_CASEFOLD FNM_IGNORECASE
#endif

#ifndef FNM_LEADING_DIR
#define FNM_LEADING_DIR FNM_PERIOD
#endif

struct U_EXPORT UServices {

   static int  getDevNull();           /* return open(/dev/null) */
   static bool isSetuidRoot();         /* UID handling: are we setuid-root...? */
   static void closeStdInputOutput();  /* move stdin and stdout to /dev/null */

   /**
    * Read data from fd
    *
    * @param timeoutMS specified the timeout value, in milliseconds.
    *        A negative value indicates no timeout, i.e. an infinite wait.
    */

   // read while not received count data

   static bool read(int fd, UString& buffer, int count = U_SINGLE_READ, int timeoutMS = -1);

   // read while received data

   static void readEOF(int fd, UString& buffer)
      {
      U_TRACE(0, "UServices::readEOF(%d,%.*S)", fd, U_STRING_TO_TRACE(buffer))

      while (UServices::read(fd, buffer, U_SINGLE_READ, -1)) {}
      }

   // generic MatchType { U_FNMATCH = 0, U_DOSMATCH = 1, U_DOSMATCH_WITH_OR = 2 };

   static bool match(const UString& s, const UString& mask)
      {
      U_TRACE(0, "UServices::match(%.*S,%.*S)", U_STRING_TO_TRACE(s), U_STRING_TO_TRACE(mask))

      bool result = u_pfn_match(U_STRING_TO_PARAM(s), U_STRING_TO_PARAM(mask), u_pfn_flags);

      U_RETURN(result);
      }

   static bool match(const UStringRep* r, const UString& mask)
      {
      U_TRACE(0, "UServices::match(%p,%.*S)", r, U_STRING_TO_TRACE(mask))

      bool result = u_pfn_match(U_STRING_TO_PARAM(*r), U_STRING_TO_PARAM(mask), u_pfn_flags);

      U_RETURN(result);
      }

   static bool matchnocase(const UString& s, const UString& mask)
      {
      U_TRACE(0, "UServices::matchnocase(%.*S,%.*S)", U_STRING_TO_TRACE(s), U_STRING_TO_TRACE(mask))

      bool result = u_pfn_match(U_STRING_TO_PARAM(s), U_STRING_TO_PARAM(mask), FNM_CASEFOLD);

      U_RETURN(result);
      }

   static bool matchnocase(const UStringRep* r, const UString& mask)
      {
      U_TRACE(0, "UServices::matchnocase(%p,%.*S)", r, U_STRING_TO_TRACE(mask))

      bool result = u_pfn_match(U_STRING_TO_PARAM(*r), U_STRING_TO_PARAM(mask), FNM_CASEFOLD);

      U_RETURN(result);
      }

   // DOS or wildcard regexpr

   static bool dosMatch(const UString& s, const UString& mask, int flags)
      {
      U_TRACE(0, "UServices::dosMatch(%.*S,%.*S,%d)", U_STRING_TO_TRACE(s), U_STRING_TO_TRACE(mask), flags)

      bool result = u_dosmatch(U_STRING_TO_PARAM(mask), U_STRING_TO_PARAM(mask), flags);

      U_RETURN(result);
      }

   // DOS or wildcard regexpr - multiple patterns separated by '|'

   static bool dosMatchWithOR(const UString& s, const UString& mask, int flags)
      {
      U_TRACE(0, "UServices::dosMatchWithOR(%.*S,%.*S,%d)", U_STRING_TO_TRACE(s), U_STRING_TO_TRACE(mask), flags)

      bool result = u_dosmatch_with_OR(U_STRING_TO_PARAM(s), U_STRING_TO_PARAM(mask), flags);

      U_RETURN(result);
      }

   static bool fnmatch(const UString& s, const UString& mask, int flags = FNM_PATHNAME | FNM_CASEFOLD)
      {
      U_TRACE(0, "UServices::fnmatch(%.*S,%.*S,%d)", U_STRING_TO_TRACE(s), U_STRING_TO_TRACE(mask), flags)

      bool result = u_fnmatch(U_STRING_TO_PARAM(s), U_STRING_TO_PARAM(mask), flags);

      U_RETURN(result);
      }

   // FTW

   static bool setFtw(const UString* dir, const char* filter = 0, uint32_t filter_len = 0);

   // manage session cookies and hashing password...

   static void generateKey();
   static unsigned char key[16];

   static UString generateToken(const UString& data, time_t expire);
   static bool    getTokenData(       UString& data, const UString& value, time_t& expire);

#ifdef HAVE_SSL
   static void generateDigest(int alg, const UString& data) { generateDigest(alg, (unsigned char*)U_STRING_TO_PARAM(data)); }
   static void generateDigest(int alg, unsigned char* data, uint32_t size);
#endif

   static void generateDigest(int alg, uint32_t keylen, unsigned char* data, uint32_t size, UString& output, int base64 = 0);
   static void generateDigest(int alg, uint32_t keylen, const UString& data,                UString& output, int base64 = 0)
      { generateDigest(alg, keylen, (unsigned char*)U_STRING_TO_PARAM(data), output, base64); }

#ifdef HAVE_LIBUUID
   // creat a new unique UUID value - 16 bytes (128 bits) long
   // return from the binary representation a 36-byte string (plus tailing '\0') of the form 1b4e28ba-2fa1-11d2-883f-0016d3cca427

   static uuid_t uuid; // typedef unsigned char uuid_t[16];

   static UString getUUID();
#endif

#ifdef HAVE_SSL
   /* setup OPENSSL standard certificate directory. The X509_STORE holds the tables etc for verification stuff.
   A X509_STORE_CTX is used while validating a single certificate. The X509_STORE has X509_LOOKUPs for looking
   up certs. The X509_STORE then calls a function to actually verify the certificate chain
   */

   static UString* CApath;
   static X509_STORE* store;

   static void setCApath(const char* _CApath);

   /* When something goes wrong, this is why */

   static int verify_error;
   static int verify_depth;            /* how far to go looking up certs */
   static X509* verify_current_cert;   /* current certificate */

   static const char* verify_status(long result);

   static const char* verify_status() { return verify_status(verify_error); }

   static void setVerifyCallback(verify_cb func)
      {
      U_TRACE(0, "UServices::setVerifyCallback(%p)", func)

      U_INTERNAL_ASSERT_POINTER(func)
      U_INTERNAL_ASSERT_POINTER(store)

      X509_STORE_set_verify_cb_func(store, func);
      }

   static UString getFileName(long hash, bool crl = false)
      {
      U_TRACE(0, "UServices::getFileName(%08x,%b)", hash, crl)

      if (CApath)
         {
         UString buffer(U_CAPACITY);

         buffer.snprintf("%.*s/%08x.%s", U_STRING_TO_TRACE(*CApath), hash, (crl ? "r0" : "0"));

         U_RETURN_STRING(buffer);
         }

      U_RETURN_STRING(UString::getStringNull());
      }

   static void getOpenSSLError(UString& buffer)
      {
      U_TRACE(0, "UServices::getOpenSSLError(%p)", &buffer)

      uint32_t size;

      (void) getOpenSSLError(buffer.data(), buffer.capacity(), &size);

      buffer.size_adjust(size);
      }

   static void releaseEngine(ENGINE* e, bool bkey);
   static int X509Callback(int ok, X509_STORE_CTX* ctx);
   static ENGINE* loadEngine(const char* id, unsigned int flags);
   static char* getOpenSSLError(char* buffer = 0, uint32_t buffer_size = 0, uint32_t* psize = 0);
   static bool setupOpenSSLStore(const char* CAfile = 0, const char* CApath = 0, int store_flags = U_STORE_FLAGS);
   static EVP_PKEY* loadKey(const UString& x, const char* format, bool _private = true, const char* password = 0, ENGINE* e = 0);

   /*
    * data   is the data to be signed
    * pkey   is the corresponsding private key
    * passwd is the corresponsding password for the private key
    */

   static UString getSignatureValue(int alg, const UString& data,                           const UString& pkey, const UString& passwd, int base64, ENGINE* e = 0);
   static bool    verifySignature(  int alg, const UString& data, const UString& signature, const UString& pkey,                                    ENGINE* e = 0);

#endif
};

#endif
