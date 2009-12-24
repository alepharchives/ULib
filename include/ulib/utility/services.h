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
#  ifdef HAVE_OPENSSL_97
#     define U_STORE_FLAGS (X509_V_FLAG_CRL_CHECK | X509_V_FLAG_CRL_CHECK_ALL)
#  else
#     define U_STORE_FLAGS 0 
#  endif
typedef int (*verify_cb)(int,X509_STORE_CTX*); /* error callback */
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
    * Read data from fd - while !(EOF|ERROR|TIMEOUT)
    *
    * @param timeoutMS specified the timeout value, in milliseconds.
    *        A negative value indicates no timeout, i.e. an infinite wait.
    */
   static uint32_t read(int fd, UString& buffer, int timeoutMS = -1);

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

   static bool DosMatch(const UString& s, const UString& mask, int flags)
      {
      U_TRACE(0, "UServices::DosMatch(%.*S,%.*S,%d)", U_STRING_TO_TRACE(s), U_STRING_TO_TRACE(mask), flags)

      bool result = u_dosmatch(U_STRING_TO_PARAM(mask), U_STRING_TO_PARAM(mask), flags);

      U_RETURN(result);
      }

   // DOS or wildcard regexpr - multiple patterns separated by '|'

   static bool DosMatchWithOR(const UString& s, const UString& mask, int flags)
      {
      U_TRACE(0, "UServices::DosMatchWithOR(%.*S,%.*S,%d)", U_STRING_TO_TRACE(s), U_STRING_TO_TRACE(mask), flags)

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

   static bool setFtw(const UString* dir, const UString* filter = 0);

   // manage stateless session cookies and hashing password...

   static void generateKey();
   static unsigned char key[16];

   static UString getTokenData(const char* token);
   static UString generateToken(UString& data, time_t expire);

#ifdef HAVE_SSL
   static void generateDigest(int alg, const UString& data) { generateDigest(alg, (unsigned char*)U_STRING_TO_PARAM(data)); }
   static void generateDigest(int alg, unsigned char* data, uint32_t size);
#endif

   static void generateDigest(int alg, uint32_t keylen, unsigned char* data, uint32_t size, UString& output, bool base64, int max_columns = 0);
   static void generateDigest(int alg, uint32_t keylen, const UString& data,                UString& output, bool base64, int max_columns = 0)
      { generateDigest(alg, keylen, (unsigned char*)U_STRING_TO_PARAM(data), output, base64, max_columns); }

   static bool    checkHMAC(int alg, unsigned char* data, uint32_t size, const UString& hmac);
   static void generateHMAC(int alg, unsigned char* data, uint32_t size,       UString& output)
      { generateDigest(alg, 16, data, size, output, false); }

#ifdef HAVE_LIBUUID
   // creat a new unique UUID value - 16 bytes (128 bits) long
   // return from the binary representation a 36-byte string (plus tailing '\0') of the form 1b4e28ba-2fa1-11d2-883f-0016d3cca427

   static uuid_t uuid; // typedef unsigned char uuid_t[16];

   static UString getUUID();
#endif


#ifdef HAVE_LIBXML2
   /* Canonical XML implementation (http://www.w3.org/TR/2001/REC-xml-c14n-20010315)
    *
    * Enum xmlC14NMode {
    *    XML_C14N_1_0           = 0 : Origianal C14N 1.0 spec
    *    XML_C14N_EXCLUSIVE_1_0 = 1 : Exclusive C14N 1.0 spec
    *    XML_C14N_1_1           = 2 : C14N 1.1 spec
    * }
    */

   static UString xmlC14N(const char* data, uint32_t size, int mode, int with_comments, unsigned char** inclusive_namespaces);

   static UString xmlC14N(const UString& data, int mode = 0) { return xmlC14N(U_STRING_TO_PARAM(data), mode, 0, 0); }
#endif

#ifdef HAVE_SSL
   /* setup OPENSSL standard certificate directory. The X509_STORE holds the tables etc for verification stuff.
   A X509_STORE_CTX is used while validating a single certificate. The X509_STORE has X509_LOOKUPs for looking
   up certs. The X509_STORE then calls a function to actually verify the certificate chain
   */

   static UString* CApath;
   static X509_STORE* store;

   /* When something goes wrong, this is why */

   static int verify_error;
   static int verify_depth;            /* how far to go looking up certs */
   static X509* verify_current_cert;   /* current certificate */

   static void setCApath(const char* _CApath)
      {
      U_TRACE(0, "UServices::setCApath(%S)", _CApath)

      U_INTERNAL_ASSERT(_CApath && *_CApath)

      if (CApath) delete CApath;

      U_NEW_ULIB_OBJECT(CApath, UString(_CApath));
      }

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

   static const char* verify_status(long result);
   static int X509Callback(int ok, X509_STORE_CTX* ctx);
   static char* getOpenSSLError(char* buffer = 0, uint32_t buffer_size = 0, uint32_t* psize = 0);
   static bool setupOpenSSLStore(const char* CAfile = 0, const char* CApath = 0, int store_flags = U_STORE_FLAGS);
   static EVP_PKEY* loadKey(const UString& x, const char* format, bool _private = true, const char* password = 0, ENGINE* e = 0);

   /*
    * data   is the data to be signed
    * pkey   is the corresponsding private key
    * passwd is the corresponsding password for the private key
    */

   static UString getSignatureValue(int alg, const UString& data, const UString& pkey, const UString& passwd, bool base64, int max_columns = 0);
#endif
};

#endif
