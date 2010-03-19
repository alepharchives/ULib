// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    crl.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/ssl/crl.h>
#include <ulib/utility/base64.h>
#include <ulib/ssl/certificate.h>
#include <ulib/utility/services.h>
#include <ulib/utility/string_ext.h>

#include <openssl/pem.h>

X509_CRL* UCrl::readCRL(const UString& x, const char* format)
{
   U_TRACE(1, "UCrl::readCRL(%.*S,%S)", U_STRING_TO_TRACE(x), format)

   BIO* in;
   UString tmp   = x;
   X509_CRL* crl = 0;

   if (format == 0) format = (x.isBinary() ? "DER" : "PEM");

   if (U_STRNCMP(format, "PEM") == 0 &&
       U_STRNCMP(x.data(), "-----BEGIN X509 CRL-----"))
      {
      unsigned length = x.size();

      UString buffer(length);

      if (UBase64::decode(x.data(), length, buffer) == false) goto next;

      tmp    = buffer;
      format = "DER";
      }

next:
   in = (BIO*) U_SYSCALL(BIO_new_mem_buf, "%p,%d", U_STRING_TO_PARAM(tmp));

   crl = (X509_CRL*) ((U_STRNCMP(format, "PEM") == 0) ? U_SYSCALL(PEM_read_bio_X509_CRL, "%p,%p,%p,%p", in, 0, 0, 0)
                                                      : U_SYSCALL(d2i_X509_CRL_bio,      "%p,%p",       in, 0));

   (void) U_SYSCALL(BIO_free, "%p", in);

   U_RETURN_POINTER(crl, X509_CRL);
}

UString UCrl::getIssuer(X509_CRL* crl, bool ldap)
{
   U_TRACE(0, "UCrl::getIssuer(%p,%b)", crl, ldap)

   U_INTERNAL_ASSERT_POINTER(crl)

   X509_NAME* n = X509_CRL_get_issuer(crl);

   return UCertificate::getName(n, ldap);
}

bool UCrl::isUpToDate() const
{
   U_TRACE(0, "UCrl::isUpToDate()")

   U_INTERNAL_ASSERT_POINTER(crl)

   int ok = X509_V_OK,
       i  = X509_cmp_time(X509_CRL_get_lastUpdate(crl), 0);

   if      (i == 0) ok = X509_V_ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD;
   else if (i  > 0) ok = X509_V_ERR_CRL_NOT_YET_VALID;
   else
      {
      if (X509_CRL_get_nextUpdate(crl))
         {
         i = X509_cmp_time(X509_CRL_get_nextUpdate(crl), 0);

         if (i < 0) ok = X509_V_ERR_CRL_HAS_EXPIRED;
         }
      }

   U_INTERNAL_DUMP("ok = %d", ok)

   U_RETURN(ok == X509_V_OK);
}

bool UCrl::isIssued(UCertificate& ca) const
{
   U_TRACE(1, "UCrl::isIssued(%p)", &ca)

   U_INTERNAL_ASSERT_POINTER(crl)

   int ok         = X509_V_OK;
   EVP_PKEY* pkey = ca.getSubjectPublicKey();

   if (pkey == 0) ok = X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY;
   else
      {
      /* Verify CRL signature */

      if (U_SYSCALL(X509_CRL_verify, "%p,%p", crl, pkey) <= 0) ok = X509_V_ERR_CRL_SIGNATURE_FAILURE;

      U_SYSCALL_VOID(EVP_PKEY_free, "%p", pkey);
      }

   U_INTERNAL_DUMP("ok = %d", ok)

   U_RETURN(ok ==  X509_V_OK);
}

unsigned UCrl::getRevokedSerials(X509_CRL* crl, long* revoked, unsigned sz)
{
   U_TRACE(0, "UCrl::getRevokedSerials(%p,%p,%u)", crl, revoked, sz)

   U_INTERNAL_ASSERT_POINTER(crl)

   unsigned i = 0;
   X509_REVOKED* rev;
   STACK_OF(X509_REVOKED)* stackRevoked = sk_X509_REVOKED_dup(X509_CRL_get_REVOKED(crl));

   while ((rev = sk_X509_REVOKED_pop(stackRevoked)) != 0)
      {
      if (i >= sz) break;

      revoked[i++] = ASN1_INTEGER_get(rev->serialNumber);
      }

      X509_REVOKED_free(rev);
   sk_X509_REVOKED_free(stackRevoked);

   U_RETURN(i);
}

UString UCrl::getFileName(X509_CRL* crl)
{
   U_TRACE(0, "UCrl::getFileName(%p)", crl)

   U_INTERNAL_ASSERT_POINTER(crl)

   long hash = X509_NAME_hash(X509_CRL_get_issuer(crl));

   UString name = UCertificate::getFileName(hash, true);

   U_RETURN_STRING(name);
}

UString UCrl::getEncoded(const char* format) const
{
   U_TRACE(0, "UCrl::getEncoded(%S)", format)

   U_INTERNAL_ASSERT_POINTER(crl)

   if (U_STRNCMP(format, "DER")    == 0 ||
       U_STRNCMP(format, "BASE64") == 0)
      {
      unsigned len = U_SYSCALL(i2d_X509_CRL, "%p,%p", crl, 0);

      UString encoding(len);

      unsigned char* temp = (unsigned char*) encoding.data();

      (void) U_SYSCALL(i2d_X509_CRL, "%p,%p", crl, &temp);

      encoding.size_adjust(len);

      if (U_STRNCMP(format, "BASE64") == 0)
         {
         UString x(len * 3 + 32U);

         UBase64::encode(encoding, x);

         U_RETURN_STRING(x);
         }

      U_RETURN_STRING(encoding);
      }
   else if (U_STRNCMP(format, "PEM") == 0)
      {
      BIO* bio = (BIO*) U_SYSCALL(BIO_new, "%p", BIO_s_mem());

      (void) U_SYSCALL(PEM_write_bio_X509_CRL, "%p,%p", bio, crl);

      UString encoding = UStringExt::BIOtoString(bio);

      U_RETURN_STRING(encoding);
      }

   U_RETURN_STRING(UString::getStringNull());
}

long UCrl::getNumber(X509_CRL* crl)
{
   U_TRACE(1, "UCrl::getNumber(%p)", crl)

   U_INTERNAL_ASSERT_POINTER(crl)

   ASN1_INTEGER* crlnum = (ASN1_INTEGER*) U_SYSCALL(X509_CRL_get_ext_d2i, "%p,%d,%p,%p", crl, NID_crl_number, 0, 0);

   long result = 0;

   if (crlnum)
      {
      result = ASN1_INTEGER_get(crlnum);

      ASN1_INTEGER_free(crlnum);
      }

   U_RETURN(result);
}

time_t UCrl::getIssueTime(X509_CRL* crl)
{
   U_TRACE(0, "UCrl::getIssueTime(%p)", crl)

   U_INTERNAL_ASSERT_POINTER(crl)

   ASN1_UTCTIME* utctime = X509_CRL_get_lastUpdate(crl);

   time_t result = UDate::getSecondFromTime((const char*)utctime->data, true, "%2u%2u%2u%2u%2u%2uZ"); // 100212124550Z

   U_RETURN(result);
}

// STREAMS

UString UCrl::print() const
{
   U_TRACE(1, "UCrl::print()")

   U_INTERNAL_ASSERT_POINTER(crl)

   BIO* bio = (BIO*) U_SYSCALL(BIO_new, "%p", BIO_s_mem());

   (void) U_SYSCALL(X509_CRL_print, "%p,%p", bio, crl);

   UString text = UStringExt::BIOtoString(bio);

   U_RETURN_STRING(text);
}

U_EXPORT ostream& operator<<(ostream& os, const UCrl& c)
{
   U_TRACE(0+256, "UCrl::operator<<(%p,%p)", &os, &c)

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

const char* UCrl::dump(bool reset) const
{
   *UObjectIO::os << "X509_CRL  " << crl;

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
