/* HCSP.c: Handle CSP engine */

#include <stdio.h>
#include <string.h>

/*
#define FILE_CONFIG
#define OPENSSL_NO_ERR
*/
#define ENGINE_DYNAMIC_SUPPORT

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN /* Exclude rarely-used stuff from Windows headers */
#  include <windows.h>
#  ifdef _WIN32
#     include <wincrypt.h>
#  else
#     define PSYSTEMTIME void*
#     include "./WinCrypt.h"
#  endif
#  include <winnls.h>
#else
#  include "./CSP.h"
#endif

#include <openssl/err.h>
#include <openssl/x509.h>
#include <openssl/engine.h>
#include <openssl/objects.h>

#ifdef DEBUG
#include <openssl/bio.h>

#  if OPENSSL_VERSION_NUMBER >= 0x0090800fL
#     define OPENSSL_d2i_TYPE const unsigned char**
#  else
#     define OPENSSL_d2i_TYPE unsigned char**
#  endif
#endif

/*****************************************************************************
 * Constants used when creating the ENGINE
 **/
static const char* engine_HCSP_id   = "HCSP";
static const char* engine_HCSP_name = "HCSP engine support";

#if defined(FULL_DEBUG) || defined(DEBUG)
static BIO* err;
static PCHAR routine;
#endif

#ifndef OPENSSL_NO_ERR
#  include "./HCSP.err"
#endif

/*****************************************************************************
 *** Function declarations and global variable definitions                 ***
 *****************************************************************************/

/*****************************************************************************
 * Functions to handle the engine
 **/
static int HCSP_destroy(ENGINE* e);
static int HCSP_init(ENGINE* e);
static int HCSP_finish(ENGINE* e);

/* static variables */

#if defined(FILE_CONFIG) || (defined(WIN32) && defined(DEBUG))
static CHAR pCryptProvider[256];
static CHAR pCryptContainer[256];
#endif

static DWORD dwProviderType = PROV_RSA_FULL;
static CHAR  pFindPara[256];
static CHAR  pSubsystemProtocol[256];

static BOOL             bInitialized;
static PBYTE            pbBuffer;
static DWORD            dwKeySpec;
static DWORD            dwSignatureLen;
static ALG_ID           Algid;
static HCRYPTKEY        hKey;
static HCRYPTHASH       hHash;
static HCRYPTPROV       hCryptProvider;
static HCERTSTORE       hCertStore;
static PCCERT_CONTEXT   pCertContext;

#ifdef FILE_CONFIG
#  include "CSP.fcfg"
#endif

/*****************************************************************************
 * RSA functions
 **/

#ifndef OPENSSL_NO_RSA

static int HCSP_rsa_sign(int type, const unsigned char* m, unsigned int len,
                         unsigned char* sigret, unsigned int* siglen, const RSA* rsa);

static int HCSP_rsa_verify(int dtype, const unsigned char* m, unsigned int len,
                           unsigned char* sigbuf, unsigned int siglen, const RSA* rsa);

/* utility functions */

static EVP_PKEY* HCSP_load_key(ENGINE*, const char*, UI_METHOD* ui_method, void* callback_data);

/*****************************************************************************
 * Our RSA method
 **/
static RSA_METHOD HCSP_rsa = {
   "HCSP RSA method",
   NULL,                /* rsa_pub_encrypt */
   NULL,                /* rsa_pub_decrypt */
   NULL,                /* rsa_priv_encrypt */
   NULL,                /* rsa_priv_decrypt */
   NULL,                /* rsa_mod_exp */
   NULL,                /* mod_exp_mont */
   NULL,                /* init */
   NULL,                /* finish */
   RSA_FLAG_SIGN_VER,   /* flags */
   NULL,                /* app_data */
   HCSP_rsa_sign,       /* rsa_sign */
   HCSP_rsa_verify      /* rsa_verify */
};

#endif

/*****************************************************************************
 * Symetric cipher and digest function registrars
 **/

/*****************************************************************************
 * DES functions
 **/

/*****************************************************************************
 * Our DES ciphers
 **/

/*****************************************************************************
 * MD functions
 **/

/*****************************************************************************
 * Our MD digests
 **/

/*****************************************************************************
 *** Function definitions                                                  ***
 *****************************************************************************/

/*****************************************************************************
 * Functions to handle the engine
 **/

static int bind_HCSP(ENGINE* e)
{
   if (!ENGINE_set_id(e, engine_HCSP_id)
    || !ENGINE_set_name(e, engine_HCSP_name)
    || !ENGINE_set_RSA(e, &HCSP_rsa)
    || !ENGINE_set_destroy_function(e, HCSP_destroy)
    || !ENGINE_set_init_function(e, HCSP_init)
    || !ENGINE_set_finish_function(e, HCSP_finish)
    || !ENGINE_set_load_pubkey_function(e, HCSP_load_key)
    || !ENGINE_set_load_privkey_function(e, HCSP_load_key)
#ifdef FILE_CONFIG
    || !ENGINE_set_ctrl_function(e, HCSP_ctrl)
    || !ENGINE_set_cmd_defns(e, HCSP_cmd_defns)
#endif
   ) return 0;

   /* Ensure the rsaref error handling is set up */

#ifndef OPENSSL_NO_ERR
   ERR_load_HCSP_strings();
#endif

   return 1;
}

#ifdef ENGINE_DYNAMIC_SUPPORT

static int bind_helper(ENGINE* e, const char* id)
{
   if (id && (strcmp(id, engine_HCSP_id) != 0)) return 0;

   if (!bind_HCSP(e)) return 0;

   return 1;
}       

IMPLEMENT_DYNAMIC_CHECK_FN()
IMPLEMENT_DYNAMIC_BIND_FN(bind_helper)

#else

static ENGINE* engine_HCSP(void)
{
   ENGINE* ret = ENGINE_new();

   if (!ret) return NULL;

   if (!bind_HCSP(ret))
      {
      ENGINE_free(ret);

      return NULL;
      }

   return ret;
}

void ENGINE_load_HCSP(void)
{
   ENGINE* toadd = engine_HCSP(); /* Copied from eng_[openssl|dyn].c */

   if (!toadd) return;

   ENGINE_add(toadd);

   ENGINE_free(toadd);

#ifndef OPENSSL_NO_ERR
   ERR_clear_error();
#endif
}

#endif

/* Initiator which is only present to make sure this engine looks available */

static int HCSP_init(ENGINE* e)
{
#if defined(FULL_DEBUG) || defined(DEBUG)
   err = BIO_new_fp(stderr, BIO_NOCLOSE);
#endif

#ifdef FULL_DEBUG
   BIO_printf(err, "Call HCSP_init(%p)\n", e);
   ERR_print_errors(err);
#endif

   return 1;
}

/* Finisher which is only present to make sure this engine looks available */

static int HCSP_finish(ENGINE* e)
{
#ifdef FULL_DEBUG
   BIO_printf(err, "Call HCSP_finish(%p)\n", e);
   ERR_print_errors(err);
#endif

   return 1;
}

/* Destructor (complements the "ENGINE_ncipher()" constructor) */

static int HCSP_destroy(ENGINE* e)
{
#ifdef FULL_DEBUG
   BIO_printf(err, "Call HCSP_destroy(%p)\n", e);
   ERR_print_errors(err);
#endif

#ifndef OPENSSL_NO_ERR
   ERR_unload_HCSP_strings();
#endif

   return 1;
}

/*******************************************************************************
 * Information function
 *******************************************************************************/

#ifndef WIN32
#  include "./CSP.func"
#endif

#ifdef DEBUG
#  include "./CSP.dbg"
#endif

/*
 * MinGW does not yet include all the needed definitions for CryptoAPI, so define here whatever extra is needed
*/

#ifdef __MINGW32__

/*
static BOOL WINAPI
(*CryptAcquireCertificatePrivateKey)(PCCERT_CONTEXT pCert, DWORD dwFlags,
                                     void* pvReserved, HCRYPTPROV* phCryptProv,
                                     DWORD* pdwKeySpec, BOOL* pfCallerFreeProv) = NULL; // to be loaded from crypt32.dll

static int mingw_load_crypto_func(void)
{
   HINSTANCE dll;
   int result = TRUE;

#ifdef DEBUG
   BIO_printf(err, "Call mingw_load_crypto_func()\n");
#endif

   if (CryptAcquireCertificatePrivateKey) return 0;

   // MinGW does not yet have full CryptoAPI support, so load the needed function here

   dll = LoadLibrary("crypt32");

   if (dll == NULL)
      {
#ifdef DEBUG
      routine = "LoadLibrary";
#endif

      goto error;
      }

   CryptAcquireCertificatePrivateKey = GetProcAddress(dll, "CryptAcquireCertificatePrivateKey");

   if (CryptAcquireCertificatePrivateKey == NULL)
      {
#ifdef DEBUG
      routine = "GetProcAddress";
#endif

      goto error;
      }

#ifdef DEBUG
   CertEnumCertificatesInStore = (void*) GetProcAddress(dll, "CertEnumCertificatesInStore");

   if (CertEnumCertificatesInStore == NULL)
      {
#ifdef DEBUG
      routine = "GetProcAddress";
#endif

      goto error;
      }
#endif

   goto end;

error:

   result = FALSE;

end:

#ifdef DEBUG
   BIO_printf(err, "Return mingw_load_crypto_func(%d)\n", result);
#endif

   return result;
}
*/
#endif

static ALG_ID HCSP_getAlgid(int type)
{
   ALG_ID Algid = CALG_MD5;

   switch (type)
      {
      case NID_md5_sha1:
         {
      // Algid = ???;
         }
      break;

      case NID_md5:
         {
      // Algid = CALG_MD5;
         }
      break;

      case NID_sha1:
         {
         Algid = CALG_SHA;
         }
      break;
      }

   return Algid;
}

static void HCSP_end(void)
{
   // Release crypto handles.

   if (hKey)            CryptDestroyKey(hKey);
   if (hHash)           CryptDestroyHash(hHash);
   if (pCertContext)    CertFreeCertificateContext(pCertContext);
   if (hCertStore)      CertCloseStore(hCertStore, 0);
   if (hCryptProvider)  CryptReleaseContext(hCryptProvider, 0);

#ifdef DEBUG
   ERR_print_errors(err);
#endif
}

static int HCSP_setContext(void)
{
   int result = TRUE;

   LPSTR a = NULL;
   LPSTR b = NULL;
   LPSTR c = "My";

#ifdef __MINGW32__
   /*
   if (!mingw_load_crypto_func())
      {
#  ifdef DEBUG
      routine = "mingw_load_crypto_func";
#  endif

      goto error;
      }
   */
#endif

#ifdef FILE_CONFIG
   readFileConfig(); /* get specified context provider, etc...  */

   if (pCryptProvider[0]     && strcasecmp(pCryptProvider,     "default")) a = pCryptProvider;
   if (pCryptContainer[0]    && strcasecmp(pCryptContainer,    "default")) b = pCryptContainer;
   if (pSubsystemProtocol[0] && strcasecmp(pSubsystemProtocol, "default")) c = pSubsystemProtocol;
#endif

#ifdef DEBUG
   strcpy(pSubsystemProtocol, c);

   BIO_printf(err, "Call HCSP_setContext()\n");
   BIO_printf(err, "pCryptProvider: \"%s\"\n", a);
   BIO_printf(err, "dwProviderType: %d\n", dwProviderType);
   BIO_printf(err, "pCryptContainer: \"%s\"\n", b);
   BIO_printf(err, "pSubsystemProtocol: \"%s\"\n", c);
   BIO_printf(err, "pFindPara: \"%s\"\n", pFindPara);
#endif

   if (!(hCertStore = CertOpenSystemStore(hCryptProvider, c)))
      {
#  ifdef DEBUG
      routine = "CertOpenSystemStore";
#  endif

      goto error;
      }

   /*
   wchar_t PWC[1024];
   MultiByteToWideChar(CP_ACP, 0, pFindPara, -1, PWC, sizeof(PWC));
   CERT_FIND_SUBJECT_STR,
   PWC,
   */

   if (!(pCertContext = CertFindCertificateInStore(
        hCertStore,
        (PKCS_7_ASN_ENCODING | X509_ASN_ENCODING),
        0,
        CERT_FIND_SUBJECT_STR_A,
        pFindPara,
        NULL)))
      {
#  ifdef DEBUG
      routine = "CertFindCertificateInStore";
#  endif

      goto error;
      }

   if (!CryptAcquireCertificatePrivateKey(
      pCertContext,
      0,
      NULL,
      &hCryptProvider,
      &dwKeySpec,
      NULL))
      {
#  ifdef DEBUG
      routine = "CryptAcquireCertificatePrivateKey";
#  endif

      goto error;
      }

#ifdef DEBUG
   printInfo();
   enumKeyContainers();
   enumAlgorithms();
   enumCertificate();

   printCertificate(pCertContext);
#endif

   bInitialized = TRUE;    /* set initialization flag */

   goto end;

error:

   result = FALSE;

end:

#ifdef DEBUG
   BIO_printf(err, "Return HCSP_setContext(%d)\n", result);
#endif

   return result;
}

static int HCSP_setHashContext(void)
{
   int result = TRUE;

#ifdef DEBUG
   BIO_printf(err, "Call HCSP_setHashContext()\n");
#endif

   /* Create a hash object. */

   if (!CryptCreateHash(hCryptProvider, Algid, 0, 0, &hHash))
      {
#ifdef DEBUG
      routine = "CryptCreateHash";
#endif

      goto error;
      }

   /* Set data to hash object. */

   if (!CryptSetHashParam(hHash, HP_HASHVAL, pbBuffer, 0))
      {
#ifdef DEBUG
      routine = "CryptSetHashParam";
#endif

      goto error;
      }

   /* Determine size of signature. */

   if (!CryptSignHash(hHash, dwKeySpec, NULL, 0, NULL, &dwSignatureLen))
      {
#  ifdef DEBUG
      routine = "CryptSignHash";
#  endif

      goto error;
      }

#ifdef DEBUG
   BIO_printf(err, "size of signature %d\n", dwSignatureLen);
#endif

   goto end;

error:

   result = FALSE;

end:

#ifdef DEBUG
   BIO_printf(err, "Return HCSP_setHashContext(%d)\n", result);
#endif

   return result;
}

static int HCSP_getKeyHandle(void)
{
   int result = TRUE;

#ifdef DEBUG
   BIO_printf(err, "Call HCSP_getKeyHandle()\n");
#endif

   /* Get handle to signature key. */

   if (!hKey)
      {
      if (!CryptGetUserKey(hCryptProvider, dwKeySpec, &hKey))
         {
#ifdef DEBUG
         routine = "CryptGetUserKey";
#endif

         goto error;
         }
      }

   goto end;

error:

   result = FALSE;

end:

#ifdef DEBUG
   BIO_printf(err, "Return HCSP_getKeyHandle(%d)\n", result);
#endif

   return result;
}

/*****************************************************************************
 * RSA functions
 **/

static int HCSP_rsa_sign(int type,
                         const unsigned char* m,
                         unsigned int len,
                         unsigned char* sigret,
                         unsigned int* siglen,
                         const RSA* rsa)
{
   BYTE* pbSignature = (BYTE*) sigret;
   DWORD dwFlags = 0L, dwLastError = 0L;  /* set last error flag to success value */

   BYTE tmp[1000];        
   int i, j, result = TRUE;

   pbBuffer = (BYTE*) m;
   dwSignatureLen = 0L;


#ifdef DEBUG
   BIO_printf(err, "Call HCSP_rsa_sign(%d)\n", type);
#endif

   if (!bInitialized &&
       !HCSP_setContext())
      {
      goto error;
      }

   Algid = HCSP_getAlgid(type);

   if (!HCSP_setHashContext())
      {
      goto error;
      }

#ifdef DEBUG
   BIO_printf(err, "siglen %d - len %d \n", *siglen, len);
#endif

   /* Sign hash object (with signature key).
   */

   pbSignature[0] = '\0';

   // dwFlags: CRYPT_NOHASHOID = 1, CRYPT_TYPE2_FORMAT = 2, CRYPT_X931_FORMAT = 4

   if (!CryptSignHash(hHash, dwKeySpec, NULL, dwFlags, pbSignature, &dwSignatureLen))
      {
#  ifdef DEBUG
      routine = "CryptSignHash";
#  endif

      goto error;
      }

   *siglen = dwSignatureLen;

#ifdef DEBUG
   BIO_printf(err, "size of signature %d\n", dwSignatureLen);
#endif

   /* little-endian... */

   for (i = 0, j = dwSignatureLen - 1; i <= j; ++i)
      {
      tmp[i] = pbSignature[j - i];
      }

   memcpy(pbSignature, tmp, dwSignatureLen);

   goto end;

error:

   result = FALSE;

#ifndef OPENSSL_NO_ERR
   dwLastError = GetLastError();

   HCSP_err(HCSP_F_RSA_SIGN, dwLastError);
#endif

end:

   HCSP_end();

#ifdef DEBUG
   BIO_printf(err, "Return HCSP_rsa_sign(%d)\n", result);
#endif

   return result;
}

static int HCSP_rsa_verify(int type,
                           const unsigned char* m,
                           unsigned int len,
                           unsigned char* sigbuf,
                           unsigned int siglen,
                           const RSA* rsa)
{
   DWORD dwLastError = 0L;
   BYTE* pbSignature = (BYTE*) sigbuf;

   BYTE tmp[1000];        
   int i, j, result = TRUE;

   pbBuffer       = (PBYTE) m;
   dwSignatureLen = 0;

#ifdef DEBUG
   BIO_printf(err, "Call HCSP_rsa_verify(%d)\n", type);
#endif

   if (!bInitialized &&
       !HCSP_setContext())
      {
      goto error;
      }

   Algid = HCSP_getAlgid(type);

   if (!HCSP_setHashContext())
      {
      goto error;
      }

#ifdef DEBUG
   BIO_printf(err, "size of signature %d - siglen %d \n", dwSignatureLen, siglen);
#endif

   /* Get handle to signature key */

   if (!HCSP_getKeyHandle())
      {
      goto error;
      }

   /* little-endian... */

   for (i = 0, j = dwSignatureLen - 1; i <= j; ++i)
      {
      tmp[i] = pbSignature[j - i];
      }

   memcpy(pbSignature, tmp, dwSignatureLen);

   /* Verify signature. */

   if (!CryptVerifySignature(hHash, pbSignature, dwSignatureLen, hKey, NULL, 0))
      {
#ifdef DEBUG
      routine = "CryptVerifySignature";
#endif

      goto error;
      }

   goto end;

error:

   result = FALSE;

#ifndef OPENSSL_NO_ERR
   dwLastError = GetLastError();

   HCSP_err(HCSP_F_RSA_VERIFY, dwLastError);
#endif

end:

   HCSP_end();

#ifdef DEBUG
   BIO_printf(err, "Return HCSP_rsa_verify(%d)\n", result);
#endif

   return result;
}

static EVP_PKEY* HCSP_load_key(ENGINE* e, const char* key_id, UI_METHOD* ui_method, void* callback_data)
{
   int i, j;
   BYTE modulus[1000];
   BYTE pbKeyBlob[1000];
   BYTE pubexp[sizeof(DWORD)];
   DWORD dwBlobLen = 1000L;
   DWORD dwLastError = 0L;

   RSA* rtmp     = RSA_new_method(e);
   EVP_PKEY* res = EVP_PKEY_new();

   typedef struct _PKBLOB {
      PUBLICKEYSTRUC  publickeystruc;
      RSAPUBKEY rsapubkey;
      BYTE modulus[1024/8];
   } PKBLOB;

   PKBLOB* pkblob = (PKBLOB*) pbKeyBlob;

#ifdef DEBUG
   BIO_printf(err, "Call HCSP_load_key(\"%s\")\n", key_id);
#endif

#ifndef FILE_CONFIG
   strcpy(pFindPara, key_id);
#endif

   if (!HCSP_setContext())
      {
      goto error;
      }

   /* Get handle to signature key. */

   if (!HCSP_getKeyHandle())
      {
      goto error;
      }

   if (!CryptExportKey(hKey, 0, PUBLICKEYBLOB, 0, pbKeyBlob, &dwBlobLen))
      {
#ifdef DEBUG
      routine = "CryptExportKey";
#endif

      goto error;
      }

   /* little-endian...
#ifdef DEBUG
   writeData((unsigned char*)pkblob->rsapubkey.pubexp, sizeof(DWORD), "pubexp");
   writeData(pkblob->modulus, pkblob->rsapubkey.bitlen/8, "modulus");
#endif
    */

   for (i = 0, j = sizeof(DWORD) - 1; i <= j; ++i)
      {
      pubexp[i] = ((unsigned char*)&(pkblob->rsapubkey.pubexp))[j - i];
      }

   for (i = 0, j = pkblob->rsapubkey.bitlen/8 - 1; i <= j; ++i)
      {
      modulus[i] = pkblob->modulus[j - i];
      }

   rtmp->e = BN_bin2bn(pubexp, sizeof(DWORD), NULL);
   rtmp->n = BN_bin2bn(modulus, pkblob->rsapubkey.bitlen/8, NULL);

#ifdef DEBUG
   BIO_printf(err, "BLOB size %d - modulus size %d \n", dwBlobLen, pkblob->rsapubkey.bitlen/8);
#endif

   EVP_PKEY_assign_RSA(res, rtmp);

   goto end;

error:

#ifndef OPENSSL_NO_ERR
   dwLastError = GetLastError();

   HCSP_err(HCSP_F_LOAD_PRIVKEY, dwLastError);
#endif

end:

#ifdef DEBUG
   BIO_printf(err, "Return HCSP_load_key(%p)\n", res);
#endif

   return res;
}

/*****************************************************************************
 * Symetric cipher and digest function registrers
 **/

/*****************************************************************************
 * DES functions
 **/

/*****************************************************************************
 * MD functions
 **/
