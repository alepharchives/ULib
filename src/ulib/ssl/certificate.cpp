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
#include <ulib/utility/base64.h>
#include <ulib/ssl/certificate.h>
#include <ulib/container/vector.h>
#include <ulib/utility/services.h>
#include <ulib/utility/string_ext.h>
#include <ulib/container/hash_map.h>

#if defined(__MINGW32__)
#  undef X509_NAME
#endif

#include <openssl/pem.h>

bool                    UCertificate::verify_result;
X509_STORE_CTX*         UCertificate::csc;
UVector<UCertificate*>* UCertificate::vcert;

X509* UCertificate::readX509(const UString& x, const char* format)
{
   U_TRACE(1, "UCertificate::readX509(%.*S,%S)", U_STRING_TO_TRACE(x), format)

   BIO* in;
   X509* _x509 = 0;
   UString tmp = x;

   if (format == 0) format = (x.isBinary() ? "DER" : "PEM");

   if (U_STREQ(format, "PEM") &&
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

   _x509 = (X509*) (U_STREQ(format, "PEM") ? U_SYSCALL(PEM_read_bio_X509, "%p,%p,%p,%p", in, 0, 0, 0)
                                           : U_SYSCALL(d2i_X509_bio,      "%p,%p",       in, 0));

   (void) U_SYSCALL(BIO_free, "%p", in);

   U_RETURN_POINTER(_x509, X509);
}

UString UCertificate::getName(X509_NAME* n, bool bldap)
{
   U_TRACE(1, "UCertificate::getName(%p,%b)", n, bldap)

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

   if (bldap)
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

      len = u_strlen(ptr);

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

bool UCertificate::isSameIssuerAndSubject() const
{
   U_TRACE(1, "UCertificate::isSameIssuerAndSubject()")

   U_INTERNAL_ASSERT_POINTER(x509)

   X509_NAME* issuer  = (X509_NAME*) U_SYSCALL(X509_get_issuer_name,  "%p", x509);
   X509_NAME* subject = (X509_NAME*) U_SYSCALL(X509_get_subject_name, "%p", x509);

   if (U_SYSCALL(X509_NAME_cmp, "%p,%p", issuer, subject) == 0) U_RETURN(true);

   U_RETURN(false);
}

UString UCertificate::getSignable(X509* _x509)
{
   U_TRACE(1, "UCertificate::getSignable(%p)", _x509)

   U_INTERNAL_ASSERT_POINTER(_x509)

   unsigned len = U_SYSCALL(i2d_X509_CINF, "%p,%p", _x509->cert_info, 0);

   UString signable(len);

   unsigned char* data = (unsigned char*) signable.data();

   (void) U_SYSCALL(i2d_X509_CINF, "%p,%p", _x509->cert_info, &data);

// len = u_strlen(data);

   signable.size_adjust(len);

   U_RETURN_STRING(signable);
}

UString UCertificate::checkForSerialNumber(long number)
{
   U_TRACE(1, "UCertificate::checkForSerialNumber(%ld)", number)

   if (number == 0) U_ERROR("serial number certificate not valid...");

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

bool UCertificate::verify(STACK_OF(X509)* chain, time_t certsVerificationTime) const
{
   U_TRACE(1, "UCertificate::verify(%p,%ld)", chain, certsVerificationTime)

   U_INTERNAL_ASSERT_POINTER(UServices::store)

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
                          "not valid after" checks); if certsVerificationTime is equal to 0 (default)
                          then we verify certificates against the system's clock "now"
   */

   if (certsVerificationTime > 0) U_SYSCALL_VOID(X509_STORE_CTX_set_time, "%p,%ld,%ld", csc, 0, certsVerificationTime);

   int rc = U_SYSCALL(X509_verify_cert, "%p", csc);

   if (certsVerificationTime > 0) U_SYSCALL_VOID(X509_STORE_CTX_cleanup, "%p", csc);

   U_RETURN(rc == 1);
}

/**
 * Retrieves the signer's certificates from this certificate,
 * it does check their validity and whether any signatures are valid.
 */

int UCertificate::verifyCallback(int ok, X509_STORE_CTX* ctx)
{
   U_TRACE(0, "UCertificate::verifyCallback(%d,%p)", ok, ctx)

   U_INTERNAL_ASSERT_POINTER(vcert)

   (void) UServices::X509Callback(ok, ctx);

   if (ok == 0) verify_result = false;
   else
      {
      U_INTERNAL_DUMP("UServices::verify_depth = %d", UServices::verify_depth)

      if (UServices::verify_depth > 0)
         {
         UCertificate* cert = U_NEW(UCertificate(UServices::verify_current_cert));

         cert->duplicate();

         vcert->push_back(cert);
         }
      }

   U_RETURN(ok);
}

unsigned UCertificate::getSignerCertificates(UVector<UCertificate*>& vec, STACK_OF(X509)* chain, time_t certsVerificationTime) const
{
   U_TRACE(0, "UCertificate::getSignerCertificates(%p,%p,%ld)", &vec, chain, certsVerificationTime)

   U_INTERNAL_ASSERT_POINTER(x509)

   verify_result = true;

   UServices::verify_depth = 20;
   UServices::setVerifyCallback(UCertificate::verifyCallback);

   vcert = &vec;

   unsigned l = vec.size();

   (void) verify(chain, certsVerificationTime);

   unsigned result = vec.size() - l;

   U_RETURN(result);
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

         (void) str.replace(buffer, len);

         U_INTERNAL_DUMP("ext[%d] = <%.*S,%.*S>", i, U_STRING_TO_TRACE(key), U_STRING_TO_TRACE(str))

         table.insert(key, str);
         }

      (void) U_SYSCALL(BIO_free, "%p", out);
      }

   U_RETURN(count);
}

U_NO_EXPORT UString UCertificate::getRevocationURI(GENERAL_NAMES* gens)
{
   U_TRACE(0, "UCertificate::getRevocationURI(%p)", gens)

   GENERAL_NAME* gen;
   UString uri(1000U);

   for (int j = 0, k = sk_GENERAL_NAME_num(gens); j < k; ++j)
      {
      gen = sk_GENERAL_NAME_value(gens, j);

      // try to grab the first one that matches

      if (gen->type == GEN_URI)
         {
         uri = (char*) U_SYSCALL(ASN1_STRING_data, "%p", gen->d.uniformResourceIdentifier);
         }
      else if (gen->type == GEN_DIRNAME)
         {
         (void) U_SYSCALL(X509_NAME_oneline, "%p,%p,%d", gen->d.directoryName, uri.data(), uri.capacity());

         uri.size_adjust();
         }

      /* unsupported generalName decoding...

      else
         {
         }
      */
      }

   U_RETURN_STRING(uri);
}

uint32_t UCertificate::getRevocationURL(UVector<UString>& vec) const
{
   U_TRACE(0, "UCertificate::getRevocationURL(%p)", &vec)

   /* X509v3 CRL Distribution Points
    *
    * This is a multi-valued extension whose options can be either in name:value pair
    * using the same form as subject alternative name or a single value representing a
    * section name containing all the distribution point fields.
    *
    * For a name:value pair a new DistributionPoint with the fullName field set to the
    * given value both the cRLissuer and reasons fields are omitted in this case.
    *
    * In the single option case the section indicated contains values for each field.
    * In this section:
    *
    * If the name is ``fullname'' the value field should contain the full name of the
    * distribution point in the same format as subject alternative name.
    *
    * If the name is ``relativename'' then the value field should contain a section
    * name whose contents represent a DN fragment to be placed in this field.
    *
    * The name ``CRLIssuer'' if present should contain a value for this field in
    * subject alternative name format.
    *
    * If the name is ``reasons'' the value field should consist of a comma separated
    * field containing the reasons. Valid reasons are:
    *
    * ``keyCompromise'', * ``CACompromise'', ``affiliationChanged'', ``superseded'',
    * ``cessationOfOperation'', ``certificateHold'', ``privilegeWithdrawn'' and
    * ``AACompromise''.
    *
    * Simple examples:
    *
    *  crlDistributionPoints=URI:http://myhost.com/myca.crl
    *  crlDistributionPoints=URI:http://my.com/my.crl,URI:http://oth.com/my.crl
    *
    *  Full distribution point example:
    *
    *  crlDistributionPoints=crldp1_section
    *
    *  [crldp1_section]
    *
    *  fullname=URI:http://myhost.com/myca.crl
    *  CRLissuer=dirName:issuer_sect
    *  reasons=keyCompromise, CACompromise
    *
    *  [issuer_sect]
    *  C=UK
    *  O=Organisation
    *  CN=Some Name
    */

   UString uri;
   X509_NAME* base;
   X509_NAME* merge;
   GENERAL_NAME* nm;
   DIST_POINT* point;
   uint32_t result, n = vec.size();

   // CRLDistributionPoints ::= SEQUENCE OF DistributionPoint

   STACK_OF(DIST_POINT)* crld = (STACK_OF(DIST_POINT)*) U_SYSCALL(X509_get_ext_d2i, "%p,%d,%p,%p", x509, NID_crl_distribution_points, 0, 0);

   for (int i = 0, j = sk_DIST_POINT_num(crld); i < j; ++i)
      {
      point = sk_DIST_POINT_value(crld, i);

      U_INTERNAL_DUMP("point->distpoint = %p point->CRLissuer = %p", point->distpoint, point->CRLissuer)

      if (point->distpoint)
         {
         // in case distributionPoint is set it can be either fullName (0) or nameRelativeToCRLIssuer (1)...

         U_INTERNAL_DUMP("point->distpoint->type = %d", point->distpoint->type)

         if (point->distpoint->type == 0) // fullName
            {
            uri = getRevocationURI(point->distpoint->name.fullname);
            }
         else // 1 nameRelativeToCRLIssuer
            {
            U_INTERNAL_ASSERT_EQUALS(sk_X509_NAME_ENTRY_num(point->distpoint->name.relativename),1)

            if (point->CRLissuer)
               {
               nm = sk_GENERAL_NAME_value(point->CRLissuer, 0);

               U_INTERNAL_ASSERT_EQUALS(nm->type,GEN_DIRNAME) // NB: it MUST be a directory name...

               base = nm->d.directoryName;
               }
            else
               {
               base = (X509_NAME*) U_SYSCALL(X509_get_issuer_name,  "%p", x509);
               }

            merge = X509_NAME_dup(base);

            (void) X509_NAME_add_entry(merge, sk_X509_NAME_ENTRY_value(point->distpoint->name.relativename, 0), -1, 0);

            uri.setBuffer(1000U);

            (void) U_SYSCALL(X509_NAME_oneline, "%p,%p,%d", merge, uri.data(), uri.capacity());

            uri.size_adjust();

            (void) X509_NAME_free(merge);
            }
         }
      else
         {
         /* RFC3280: If the distributionPoint field is omitted, cRLIssuer MUST be present
          * and include a Name corresponding to an X.500 or LDAP directory entry where the CRL is located
          */

         if (point->CRLissuer) uri = getRevocationURI(point->CRLissuer);

         // either distributionPoint or cRLIssuer MUST be present..
         // this distributionPoint is sick, let's try another one
         }

      if (uri.empty() == false)
         {
         vec.push_back(uri);

         uri.clear();
         }
      }

   result = (vec.size() - n);

   U_RETURN(result);
}

uint32_t UCertificate::getCAIssuers(UVector<UString>& vec) const
{
   U_TRACE(0, "UCertificate::getCAIssuers(%p)", &vec)

   /* Authority Info Access
    *
    * The authority information access extension gives details about how to access
    * certain information relating to the CA. Its syntax is accessOID;location where
    * location has the same syntax as subject alternative name (except that email:copy
    * is not supported). accessOID can be any valid OID but only certain values are meaningful,
    * for example OCSP and caIssuers.
    *
    * Example:
    *
    * authorityInfoAccess = OCSP;URI:http://ocsp.my.host/
    * authorityInfoAccess = caIssuers;URI:http://my.ca/ca.html
    */

   UString uri;
   ACCESS_DESCRIPTION* ad;

   // try to extract an AuthorityInfoAccessSyntax extension from x509

   AUTHORITY_INFO_ACCESS* aia = (AUTHORITY_INFO_ACCESS*) U_SYSCALL(X509_get_ext_d2i, "%p,%d,%p,%p", x509, NID_info_access, 0, 0);

   // AuthorityInfoAccessSyntax ::= SEQUENCE OF AccessDescription

   for (int i = 0, j = sk_ACCESS_DESCRIPTION_num(aia); i < j; ++i)
      {
      ad = sk_ACCESS_DESCRIPTION_value(aia, i);

      U_INTERNAL_DUMP("ad->method = %d ad->location->type = %d", ad->method, ad->location->type)

      /* we are interested in id-ad-caIssuers accessMethod only,
       * extract general name from accessLocation. RFC3280 states:
       * "where the information is available via HTTP, FTP, or LDAP, accessLocation MUST be a uniformResourceIdentifier"
       */

      if (OBJ_obj2nid(ad->method) == NID_ad_ca_issuers &&
          ad->location->type      == GEN_URI)
         {
         uri = (char*) U_SYSCALL(ASN1_STRING_data, "%p", ad->location->d.uniformResourceIdentifier);

         if (uri.empty() == false) vec.push_back(uri);
         }
      }

   uint32_t result, n = vec.size();

   result = (vec.size() - n);

   U_RETURN(result);
}

UString UCertificate::getEncoded(const char* format) const
{
   U_TRACE(1, "UCertificate::getEncoded(%S)", format)

   U_INTERNAL_ASSERT_POINTER(x509)

   if (U_STREQ(format, "DER") ||
       U_STREQ(format, "BASE64"))
      {
      unsigned len = U_SYSCALL(i2d_X509, "%p,%p", x509, 0);

      UString encoding(len);

      unsigned char* temp = (unsigned char*) encoding.data();

      (void) U_SYSCALL(i2d_X509, "%p,%p", x509, &temp);

      encoding.size_adjust(len);

      if (U_STREQ(format, "BASE64"))
         {
         UString x(len * 3 + 32U);

         UBase64::encode(encoding, x);

         U_RETURN_STRING(x);
         }

      U_RETURN_STRING(encoding);
      }
   else if (U_STREQ(format, "PEM"))
      {
      BIO* bio = (BIO*) U_SYSCALL(BIO_new, "%p", BIO_s_mem());

      (void) U_SYSCALL(PEM_write_bio_X509, "%p,%p", bio, x509);

      UString encoding = UStringExt::BIOtoString(bio);

      U_RETURN_STRING(encoding);
      }

   U_RETURN_STRING(UString::getStringNull());
}

UString UCertificate::getModulus() const
{
   U_TRACE(1, "UCertificate::getModulus()")

   U_INTERNAL_ASSERT_POINTER(x509)

   unsigned char buffer[4096];

   EVP_PKEY* pkey = getSubjectPublicKey();

   BIGNUM* bn  = (pkey->type == EVP_PKEY_RSA ? pkey->pkey.rsa->n
                                             : pkey->pkey.dsa->pub_key); // EVP_PKEY_DSA

   int len = U_SYSCALL(BN_bn2bin, "%p,%p", bn, buffer);

   UString x(len * 3 + 32U);

   UBase64::encode(buffer, len, x);

   U_SYSCALL_VOID(EVP_PKEY_free, "%p", pkey);

   U_RETURN_STRING(x);
}

UString UCertificate::getExponent() const
{
   U_TRACE(1, "UCertificate::getExponent()")

   U_INTERNAL_ASSERT_POINTER(x509)

   unsigned char buffer[16];

   EVP_PKEY* pkey = getSubjectPublicKey();

   BIGNUM* bn  = (pkey->type == EVP_PKEY_RSA ? pkey->pkey.rsa->e
                                             : pkey->pkey.dsa->pub_key); // EVP_PKEY_DSA

   int len = U_SYSCALL(BN_bn2bin, "%p,%p", bn, buffer);

   UString x(len * 3 + 32U);

   UBase64::encode(buffer, len, x);

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
