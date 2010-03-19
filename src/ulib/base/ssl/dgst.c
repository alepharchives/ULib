// ============================================================================
//
// = LIBRARY
//    ulibase - c++ library
//
// = FILENAME
//    dgst.c
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/base/ssl/dgst.h>
#include <ulib/base/coder/base64.h>

UHashType      u_hashType;                   /* What type of hash is this? */
EVP_PKEY*      u_pkey;                       /* private key to sign the digest */
EVP_MD_CTX     u_mdctx;                      /* Context for digest */
const EVP_MD*  u_md;                         /* Digest instance */
unsigned char  u_mdValue[U_MAX_HASH_SIZE];   /* Final output */
uint32_t       u_mdLen;                      /* Length of digest */

HMAC_CTX       u_hctx;                       /* Context for HMAC */
const char*    u_hmac_key;                   /* The loaded key */
uint32_t       u_hmac_keylen;                /* The loaded key length */

void u_dgst_hexdump(unsigned char* buf)
{
   uint32_t i;
   unsigned char c;

   U_INTERNAL_TRACE("u_dgst_hexdump(%p)", buf)

   /* Copy to output buffer */

   for (i = 0; i < u_mdLen; ++i)
      {
      c = u_mdValue[i];

      *buf++ = u_hex_lower[(c >> 4) & 0x0f];
      *buf++ = u_hex_lower[(c     ) & 0x0f];

   // (void) sprintf((char*)buf, "%02x", u_mdValue[i]);
      }
}

int u_dgst_get_algoritm(const char* alg)
{
   int result = -1;

   U_INTERNAL_TRACE("u_dgst_get_algoritm(%s)", alg)

        if (U_STREQ(alg, "md5"))       result = U_HASH_MD5;
#ifndef OPENSSL_NO_MD2
   else if (U_STREQ(alg, "md2"))       result = U_HASH_MD2;
#endif
#ifndef OPENSSL_NO_SHA0
   else if (U_STREQ(alg, "sha"))       result = U_HASH_SHA;
#endif
   else if (U_STREQ(alg, "sha1"))      result = U_HASH_SHA1;
#ifdef HAVE_OPENSSL_98
   else if (U_STREQ(alg, "sha224"))    result = U_HASH_SHA224;
   else if (U_STREQ(alg, "sha256"))    result = U_HASH_SHA256;
   else if (U_STREQ(alg, "sha384"))    result = U_HASH_SHA384;
   else if (U_STREQ(alg, "sha512"))    result = U_HASH_SHA512;
#endif
#ifndef OPENSSL_NO_MDC2
   else if (U_STREQ(alg, "mdc2"))      result = U_HASH_MDC2;
#endif
#ifndef OPENSSL_NO_RMD160
   else if (U_STREQ(alg, "ripemd160")) result = U_HASH_RIPEMD160;
#endif

   return result;
}

void u_dgst_algoritm(int alg)
{
   U_INTERNAL_TRACE("u_dgst_algoritm(%d)", alg)

   switch (alg)
      {
      case U_HASH_MD5:       u_md = EVP_md5();        break;
#  ifndef OPENSSL_NO_MD2
      case U_HASH_MD2:       u_md = EVP_md2();        break;
#  endif
#  ifndef OPENSSL_NO_SHA0
      case U_HASH_SHA:       u_md = EVP_sha();        break;
#  endif
      case U_HASH_SHA1:      u_md = EVP_sha1();       break;
#  ifdef HAVE_OPENSSL_98
      case U_HASH_SHA224:    u_md = EVP_sha224();     break;
      case U_HASH_SHA256:    u_md = EVP_sha256();     break;
      case U_HASH_SHA384:    u_md = EVP_sha384();     break;
      case U_HASH_SHA512:    u_md = EVP_sha512();     break;
#  endif
#  ifndef OPENSSL_NO_MDC2
      case U_HASH_MDC2:      u_md = EVP_mdc2();       break;
#  endif
#  ifndef OPENSSL_NO_RMD160
      case U_HASH_RIPEMD160: u_md = EVP_ripemd160();  break;
#  endif
      default:               u_md = 0;
      }

   if (u_md == 0) U_ERROR("loading digest algorithm '%d' failed", alg);

   u_hashType = alg;
}

void u_dgst_init(int alg, const char* key, uint32_t keylen)
{
   U_INTERNAL_TRACE("u_dgst_init(%d,%.*s,%u)", alg, keylen, key, keylen)

   u_dgst_algoritm(alg);

   if (keylen)
      {
      u_hmac_key    = key;
      u_hmac_keylen = keylen;

      HMAC_CTX_cleanup(&u_hctx);
      HMAC_CTX_init(&u_hctx);
      HMAC_Init_ex(&u_hctx, key, keylen, u_md, NULL);
      }
   else
      {
      (void) EVP_MD_CTX_cleanup(&u_mdctx);
      (void) EVP_DigestInit(&u_mdctx, u_md);
      }
}

void u_dgst_reset(void) /* Reset the hash */
{
   U_INTERNAL_TRACE("u_dgst_reset()", 0)

   U_INTERNAL_PRINT("alg = %d", u_hashType)

   U_INTERNAL_ASSERT_POINTER(u_md)

   if (u_hmac_keylen)
      {
      HMAC_CTX_cleanup(&u_hctx);

      HMAC_CTX_init(&u_hctx);

      HMAC_Init_ex(&u_hctx, u_hmac_key, u_hmac_keylen, u_md, NULL);
      }
   else
      {
      (void) EVP_MD_CTX_cleanup(&u_mdctx);

      (void) EVP_DigestInit(&u_mdctx, u_md);
      }
}

uint32_t u_dgst_finish(unsigned char* hash, int base64) /* Finish and get hash */
{
   uint32_t len;

   U_INTERNAL_TRACE("u_dgst_finish(%p,%d)", hash, base64)

   /* Finish up and copy out hash, returning the length */

   if (u_hmac_keylen)
      {
      HMAC_Final(&u_hctx, u_mdValue, &u_mdLen);
      }
   else
      {
      (void) EVP_DigestFinal(&u_mdctx, u_mdValue, &u_mdLen);
      }

   U_INTERNAL_ASSERT_MINOR(u_mdLen,U_MAX_HASH_SIZE)

   if (hash == NULL) return u_mdLen;

   if (base64) len = u_base64_encode(u_mdValue, u_mdLen, hash);
   else
      {
      len = u_mdLen * 2;

      u_dgst_hexdump(hash);
      }

   return len;
}

/* The EVP signature routines are a high level interface to digital signatures */

void u_dgst_sign_init(int alg, ENGINE* impl)
{
   U_INTERNAL_TRACE("u_dgst_sign_init(%d,%p)", alg, impl)

   u_dgst_algoritm(alg);

   (void) EVP_MD_CTX_cleanup(&u_mdctx);

   EVP_MD_CTX_init(&u_mdctx);

   /* sets up signing context ctx to use digest type from ENGINE impl.
    * u_mdctx must be initialized with EVP_MD_CTX_init() before calling this function. */

   (void) EVP_SignInit_ex(&u_mdctx, u_md, impl);
}

void u_dgst_verify_init(int alg, ENGINE* impl)
{
   U_INTERNAL_TRACE("u_dgst_verify_init(%d,%p)", alg, impl)

   u_dgst_algoritm(alg);

   (void) EVP_MD_CTX_cleanup(&u_mdctx);

   EVP_MD_CTX_init(&u_mdctx);

   /* sets up signing context ctx to use digest type from ENGINE impl.
    * u_mdctx must be initialized with EVP_MD_CTX_init() before calling this function. */

   (void) EVP_VerifyInit_ex(&u_mdctx, u_md, impl);
}

uint32_t u_dgst_sign_finish(unsigned char* sig, int base64) /* Finish and get signature */
{
   uint32_t len;

   U_INTERNAL_TRACE("u_dgst_sign_finish(%p,%d)", sig, base64)

   U_INTERNAL_ASSERT_POINTER(u_pkey)

   (void) EVP_SignFinal(&u_mdctx, u_mdValue, &u_mdLen, u_pkey);

   U_INTERNAL_ASSERT_MINOR(u_mdLen,U_MAX_HASH_SIZE)

   if (sig == NULL) return u_mdLen;

   if (base64) len = u_base64_encode(u_mdValue, u_mdLen, sig);
   else
      {
      len = u_mdLen * 2;

      u_dgst_hexdump(sig);
      }

   return len;
}

int u_dgst_verify_finish(unsigned char* sigbuf, uint32_t siglen)
{
   int ret;

   U_INTERNAL_TRACE("u_dgst_verify_finish(%p,%u)", sigbuf, siglen)

   U_INTERNAL_ASSERT_POINTER(u_pkey)

   ret = EVP_VerifyFinal(&u_mdctx, sigbuf, siglen, u_pkey);

   return ret;
}
