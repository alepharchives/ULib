// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    certificate.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/file.h>
#include <ulib/ssl/signature.h>
#include <ulib/utility/base64.h>
#include <ulib/ssl/certificate.h>
#include <ulib/utility/services.h>
#include <ulib/utility/string_ext.h>
#include <ulib/container/hash_map.h>

#include <openssl/pem.h>
#include <openssl/x509v3.h>

X509_STORE_CTX* UCertificate::csc;

X509* UCertificate::readX509(const UString& x, const char* format)
{
   U_TRACE(1, "UCertificate::readX509(%.*S,%S)", U_STRING_TO_TRACE(x), format)

   BIO* in;
   X509* x509  = 0;
   UString tmp = x;

   if (format == 0) format = (x.isBinary() ? "DER" : "PEM");

   if (U_STRNCMP(format, "PEM") == 0 &&
       U_STRNCMP(x.data(), "-----BEGIN CERTIFICATE-----"))
      {
      unsigned length = x.size();

      UString buffer(length);

      if (UBase64::decode(x.data(), length, buffer) == false) goto next;

      tmp    = buffer;
      format = "DER";
      }

next:
   in = (BIO*) U_SYSCALL(BIO_new_mem_buf, "%p,%d", U_STRING_TO_PARAM(tmp));

   x509 = (X509*) ((U_STRNCMP(format, "PEM") == 0) ? U_SYSCALL(PEM_read_bio_X509, "%p,%p,%p,%p", in, 0, 0, 0)
                                                   : U_SYSCALL(d2i_X509_bio,      "%p,%p",       in, 0));

   (void) U_SYSCALL(BIO_free, "%p", in);

   U_RETURN_POINTER(x509, X509);
}

UString UCertificate::getName(X509_NAME* n, bool ldap)
{
   U_TRACE(1, "UCertificate::getName(%p,%b)", n, ldap)

   U_INTERNAL_ASSERT_POINTER(n)

#ifdef DEBUG // Get X509_NAME information from Issuer || Subject name
   #  ifndef    NID_uniqueIdentifier
   #     define NID_uniqueIdentifier 102
   #  endif

   char buf[256];
        buf[255] = '\0';

#  define U_X509_NAME_DUMP(id,str) if (X509_NAME_get_text_by_NID(n,id,buf,256)>0) U_INTERNAL_DUMP(str" = %S",buf)

   U_X509_NAME_DUMP(NID_commonName,            "commonName             (CN)") // CN - commonName
   U_X509_NAME_DUMP(NID_countryName,           "countryName             (C)") // C  - countryName
   U_X509_NAME_DUMP(NID_localityName,          "localityName            (L)") // L  - localityName
   U_X509_NAME_DUMP(NID_stateOrProvinceName,   "stateOrProvinceName    (ST)") // ST - stateOrProvinceName
   U_X509_NAME_DUMP(NID_organizationName,      "organizationName        (O)") // O  - organizationName
   U_X509_NAME_DUMP(NID_organizationalUnitName,"organizationalUnitName (OU)") // OU - organizationalUnitName
   U_X509_NAME_DUMP(NID_title,                 "title                   (T)") // T  - title
   U_X509_NAME_DUMP(NID_initials,              "initials                (I)") // I  - initials
   U_X509_NAME_DUMP(NID_givenName,             "givenName               (G)") // G  - givenName
   U_X509_NAME_DUMP(NID_surname,               "surname                 (S)") // S  - surname
   U_X509_NAME_DUMP(NID_description,           "description             (D)") // D  - description
   U_X509_NAME_DUMP(NID_uniqueIdentifier,      "uniqueIdentifier      (UID)") // UID - uniqueIdentifier
   U_X509_NAME_DUMP(NID_pkcs9_emailAddress,    "emailAddress        (Email)") // Email - emailAddress
#endif

   if (ldap)
      {
      BIO* bio = (BIO*) U_SYSCALL(BIO_new, "%p", BIO_s_mem());

      int res = U_SYSCALL(X509_NAME_print_ex, "%p,%p,%d,%ld", bio, n, 0, XN_FLAG_COMPAT);

      if (res == -1) (void) U_SYSCALL(BIO_free, "%p", bio);
      else
         {
         UString name = UStringExt::BIOtoString(bio);

         U_RETURN_STRING(name);
         }

      U_RETURN_STRING(UString::getStringNull());
      }
   else
      {
      unsigned len = U_SYSCALL(i2d_X509_NAME, "%p,%p", n, 0);

      UString name(len);
      char* ptr = name.data();

      (void) U_SYSCALL(X509_NAME_oneline, "%p,%p,%d", n, ptr, name.capacity());

      len = strlen(ptr);

      name.size_adjust(len);

      U_RETURN_STRING(name);
      }
}

bool UCertificate::isIssued(const UCertificate& ca) const
{
   U_TRACE(1, "UCertificate::isIssued(%p)", &ca)

   U_INTERNAL_ASSERT_POINTER(x509)

   int ok = U_SYSCALL(X509_check_issued, "%p,%p", ca.x509, x509);

   if (ok == X509_V_OK)
      {
      EVP_PKEY* pkey = ca.getSubjectPublicKey();

      if (pkey == 0) ok = X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY;
      else
         {
         // signature verification

         if (U_SYSCALL(X509_verify, "%p,%p", x509, pkey) <= 0) ok = X509_V_ERR_CERT_SIGNATURE_FAILURE;

         U_SYSCALL_VOID(EVP_PKEY_free, "%p", pkey);
         }

      U_INTERNAL_DUMP("ok = %d", ok)

      U_RETURN(ok ==  X509_V_OK);
      }

   U_RETURN(false);
}

bool UCertificate::isSelfSigned() const
{
   U_TRACE(1, "UCertificate::isSelfSigned()")

   U_INTERNAL_ASSERT_POINTER(x509)

   X509_NAME* issuer  = (X509_NAME*) U_SYSCALL(X509_get_issuer_name,  "%p", x509);
   X509_NAME* subject = (X509_NAME*) U_SYSCALL(X509_get_subject_name, "%p", x509);

   if (U_SYSCALL(X509_NAME_cmp, "%p,%p", issuer, subject) == 0 && isIssued(*this)) U_RETURN(true);

   U_RETURN(false);
}

UString UCertificate::getSignable(X509* x509)
{
   U_TRACE(1, "UCertificate::getSignable(%p)", x509)

   U_INTERNAL_ASSERT_POINTER(x509)

   unsigned len = U_SYSCALL(i2d_X509_CINF, "%p,%p", x509->cert_info, 0);

   UString signable(len);

   unsigned char* data = (unsigned char*) signable.data();

   (void) U_SYSCALL(i2d_X509_CINF, "%p,%p", x509->cert_info, &data);

// len = strlen(data);

   signable.size_adjust(len);

   U_RETURN_STRING(signable);
}

UString UCertificate::checkForSerialNumber(long number)
{
   U_TRACE(1, "UCertificate::checkForSerialNumber(%ld)", number)

   if (number == 0) U_ERROR("serial number certificate not valid...", 0);

   ASN1_INTEGER* a = ASN1_INTEGER_new();

   (void) U_SYSCALL(ASN1_INTEGER_set, "%p,%ld", a, number);

   BIGNUM* bn = (BIGNUM*) U_SYSCALL(ASN1_INTEGER_to_BN, "%p,%p", a, NULL);

   char* itmp = (char*) U_SYSCALL(BN_bn2hex, "%p", bn);

   U_SYSCALL_VOID(BN_free, "%p", bn);

   U_SYSCALL_VOID(ASN1_INTEGER_free, "%p", a);

   UString serial((void*)itmp);

   U_SYSCALL_VOID(OPENSSL_free, "%p", itmp);

   U_RETURN_STRING(serial);
}

bool UCertificate::verify(EVP_PKEY* publicKey) const
{
   U_TRACE(0, "UCertificate::verify(%p)", publicKey)

   const char* alg = getSignatureAlgorithm().c_str();

   USignature sig(alg);

   sig.initVerify(publicKey);

   UString signable = getSignable();

   sig.update(signable);

   UString signature = getSignature();

   bool result = sig.verify(signature);

   U_RETURN(result);
}

bool UCertificate::verify(STACK_OF(X509)* chain, time_t certsVerificationTime) const
{
   U_TRACE(1, "UCertificate::verify(%p,%ld)", chain, certsVerificationTime)

   if (csc == 0) csc = (X509_STORE_CTX*) U_SYSCALL_NO_PARAM(X509_STORE_CTX_new); // create an X509 store context

   U_INTERNAL_ASSERT_POINTER(csc)
   U_INTERNAL_ASSERT_POINTER(x509)

   /*
   The fourth parameter is a collection of any certificates that might help the verify process.
   It will normally be searched for untrusted CAs. It can contain other certificates in the expected path,
   unrelated certificates or none at all
   */

   // initialize an X509 STORE context

#ifdef HAVE_OPENSSL_97
   (void) U_SYSCALL(X509_STORE_CTX_init, "%p,%p,%p,%p", csc, UServices::store, x509, chain);
#else
   U_SYSCALL_VOID(  X509_STORE_CTX_init, "%p,%p,%p,%p", csc, UServices::store, x509, chain);
#endif

   /*
   certsVerificationTime: the time to use for X509 certificates verification ("not valid before" and
   "not valid after" checks); if certsVerificationTime is equal to 0 (default) then we verify certificates
   against the system's clock "now"
   */

   if (certsVerificationTime > 0) U_SYSCALL_VOID(X509_STORE_CTX_set_time, "%p,%ld,%ld", csc, 0, certsVerificationTime);

   int rc = U_SYSCALL(X509_verify_cert, "%p", csc);

   if (certsVerificationTime > 0) U_SYSCALL_VOID(X509_STORE_CTX_cleanup, "%p", csc);

   U_RETURN(rc == 1);
}

/*
 * Gets X509v3 extensions as array of X509Ext objects
 */

int UCertificate::getExtensions(UHashMap<UString>& table) const
{
   U_TRACE(1, "UCertificate::getExtensions(%p)", &table)

   U_INTERNAL_ASSERT_POINTER(x509)

   int count = U_SYSCALL(X509_get_ext_count, "%p", x509);

   if (count > 0)
      {
      int len;
      UString key, str;
      char buffer[4096];
      X509_EXTENSION* ext;

      BIO* out = (BIO*) U_SYSCALL(BIO_new, "%p", BIO_s_mem());

      if (table.capacity() == 0) table.allocate();

      for (int i = 0; i < count; ++i)
         {
         ext = X509_get_ext(x509, i); /* NO DUP - don't free! */

         U_INTERNAL_ASSERT_POINTER(ext)

         key.assign(OBJ_nid2ln(OBJ_obj2nid(ext->object)));

         if (!X509V3_EXT_print(out, ext, 0, 0)) M_ASN1_OCTET_STRING_print(out, ext->value);

         len = BIO_read(out, buffer, sizeof(buffer));

         str.replace(buffer, len);

         U_INTERNAL_DUMP("ext[%d] = <%.*S,%.*S>", i, U_STRING_TO_TRACE(key), U_STRING_TO_TRACE(str))

         table.insert(key, str);
         }

      BIO_free(out);
      }

   U_RETURN(count);
}

UString UCertificate::getRevocationURL(const char* ext_id) const
{
   U_TRACE(0, "UCertificate::getRevocationURL(%S)", ext_id)

   UString crl;
   UHashMap<UString> table;

   if (getExtensions(table))
      {
      UString id = UString(ext_id);

      crl = table[id];

      table.clear();
      table.deallocate();
      }

   U_RETURN_STRING(crl);
}

UString UCertificate::getEncoded(const char* format, int max_columns) const
{
   U_TRACE(1, "UCertificate::getEncoded(%S,%d)", format, max_columns)

   U_INTERNAL_ASSERT_POINTER(x509)

   if (U_STRNCMP(format, "DER")    == 0 ||
       U_STRNCMP(format, "BASE64") == 0)
      {
      unsigned len = U_SYSCALL(i2d_X509, "%p,%p", x509, 0);

      UString encoding(len);

      unsigned char* temp = (unsigned char*) encoding.data();

      (void) U_SYSCALL(i2d_X509, "%p,%p", x509, &temp);

      encoding.size_adjust(len);

      if (U_STRNCMP(format, "BASE64") == 0)
         {
         UString x(len * 3 + 32U);

         UBase64::encode(encoding, x, max_columns);

         U_RETURN_STRING(x);
         }

      U_RETURN_STRING(encoding);
      }
   else if (U_STRNCMP(format, "PEM") == 0)
      {
      BIO* bio = (BIO*) U_SYSCALL(BIO_new, "%p", BIO_s_mem());

      (void) U_SYSCALL(PEM_write_bio_X509, "%p,%p", bio, x509);

      UString encoding = UStringExt::BIOtoString(bio);

      U_RETURN_STRING(encoding);
      }

   U_RETURN_STRING(UString::getStringNull());
}

UString UCertificate::getModulus(int max_columns) const
{
   U_TRACE(1, "UCertificate::getModulus(%d)", max_columns)

   U_INTERNAL_ASSERT_POINTER(x509)

   unsigned char buffer[4096];

   EVP_PKEY* pkey = getSubjectPublicKey();

   BIGNUM* bn  = (pkey->type == EVP_PKEY_RSA ? pkey->pkey.rsa->n
                                             : pkey->pkey.dsa->pub_key); // EVP_PKEY_DSA

   int len = U_SYSCALL(BN_bn2bin, "%p,%p", bn, buffer);

   UString x(len * 3 + 32U);

   UBase64::encode(buffer, len, x, max_columns);

   U_SYSCALL_VOID(EVP_PKEY_free, "%p", pkey);

   U_RETURN_STRING(x);
}

UString UCertificate::getExponent(int max_columns) const
{
   U_TRACE(1, "UCertificate::getExponent(%d)", max_columns)

   U_INTERNAL_ASSERT_POINTER(x509)

   unsigned char buffer[16];

   EVP_PKEY* pkey = getSubjectPublicKey();

   BIGNUM* bn  = (pkey->type == EVP_PKEY_RSA ? pkey->pkey.rsa->e
                                             : pkey->pkey.dsa->pub_key); // EVP_PKEY_DSA

   int len = U_SYSCALL(BN_bn2bin, "%p,%p", bn, buffer);

   UString x(len * 3 + 32U);

   UBase64::encode(buffer, len, x, max_columns);

   U_SYSCALL_VOID(EVP_PKEY_free, "%p", pkey);

   U_RETURN_STRING(x);
}

UString UCertificate::getFileName(long hash, bool crl, bool* exist)
{
   U_TRACE(0, "UCertificate::getFileName(%08x,%b,%p)", hash, crl, exist)

   UString name = UServices::getFileName(hash, crl);

   if (name.empty() == false)
      {
      UString buffer = UFile::getRealPath(name.c_str());

      if (buffer.empty() == false)
         {
         if (exist) *exist = true;

         U_RETURN_STRING(buffer);
         }
      }

   if (exist) *exist = false;

   U_RETURN_STRING(name);
}

STACK_OF(X509)* UCertificate::loadCerts(const UString& x)
{
   U_TRACE(1, "UCertificate::loadCerts(%.*S)", U_STRING_TO_TRACE(x))

   BIO* certs = (BIO*) U_SYSCALL(BIO_new_mem_buf, "%p,%d", U_STRING_TO_PARAM(x));

   STACK_OF(X509)* othercerts = sk_X509_new_null();

   STACK_OF(X509_INFO)* allcerts = (STACK_OF(X509_INFO)*) U_SYSCALL(PEM_X509_INFO_read_bio, "%p,%p,%p,%p", certs, 0, 0, 0);

   X509_INFO* xi;
   int n = U_SYSCALL(sk_X509_INFO_num, "%p", allcerts);

   for (int i = 0; i < n; ++i)
      {
      xi = sk_X509_INFO_value(allcerts, i);

      if (xi->x509)
         {
         sk_X509_push(othercerts, xi->x509);

         xi->x509 = 0;
         }
      }

   if (allcerts) sk_X509_INFO_pop_free(allcerts, X509_INFO_free);

   (void) U_SYSCALL(BIO_free, "%p", certs);

   U_RETURN_POINTER(othercerts, STACK_OF(X509));
}

// STREAMS

UString UCertificate::print(unsigned long nmflag, unsigned long cflag) const
{
   U_TRACE(1, "UCertificate::print(%lu,%lu)", nmflag, cflag)

   BIO* bio = (BIO*) U_SYSCALL(BIO_new, "%p", BIO_s_mem());

#ifdef HAVE_OPENSSL_97
   (void) U_SYSCALL(X509_print_ex, "%p,%p,%lu,%lu", bio, x509, nmflag, cflag);
#else
   (void) U_SYSCALL(X509_print,    "%p,%p", bio, x509);
#endif

   UString text = UStringExt::BIOtoString(bio);

   U_RETURN_STRING(text);
}

U_EXPORT ostream& operator<<(ostream& os, const UCertificate& c)
{
   U_TRACE(0+256, "UCertificate::operator<<(%p,%p)", &os, &c)

   os.put('{');
   os.put(' ');

   UString text = c.print();

   (void) os.write(text.data(), text.size());

   os.put(' ');
   os.put('}');

   return os;
}

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UCertificate::dump(bool reset) const
{
   *UObjectIO::os << "x509   " << x509;

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
