// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    pkcs10.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/ssl/pkcs10.h>
#include <ulib/ssl/signature.h>
#include <ulib/utility/base64.h>
#include <ulib/utility/string_ext.h>

#include <openssl/pem.h>

X509_REQ* UPKCS10::readPKCS10(const UString& x, const char* format)
{
   U_TRACE(1, "UPKCS10::readPKCS10(%.*S,%S)", U_STRING_TO_TRACE(x), format)

   BIO* in;
   UString tmp       = x;
   X509_REQ* request = 0;

   if (format == 0) format = (x.isBinary() ? "DER" : "PEM");

   if (U_STRNCMP(format, "PEM") == 0 &&
       U_STRNCMP(x.data(), "-----BEGIN CERTIFICATE REQUEST-----"))
      {
      unsigned length = x.size();

      UString buffer(length);

      if (UBase64::decode(x.data(), length, buffer) == false) goto next;

      tmp    = buffer;
      format = "DER";
      }

next:
   in = (BIO*) U_SYSCALL(BIO_new_mem_buf, "%p,%d", U_STRING_TO_PARAM(tmp));

   request = (X509_REQ*) ((U_STRNCMP(format, "PEM") == 0) ? U_SYSCALL(PEM_read_bio_X509_REQ, "%p,%p,%p,%p", in, 0, 0, 0)
                                                          : U_SYSCALL(d2i_X509_REQ_bio,      "%p,%p",       in, 0));

   (void) U_SYSCALL(BIO_free, "%p", in);

   U_RETURN_POINTER(request, X509_REQ);
}

UString UPKCS10::getSubject(X509_REQ* request)
{
   U_TRACE(1, "UPKCS10::getSubject(%p)", request)

   U_INTERNAL_ASSERT_POINTER(request)

   X509_NAME* name = X509_REQ_get_subject_name(request);
   unsigned len    = U_SYSCALL(i2d_X509_NAME, "%p,%p", name, 0);

   UString subject(len);
   char* ptr = subject.data();

   (void) U_SYSCALL(X509_NAME_oneline, "%p,%p,%d", name, ptr, subject.capacity() );

   len = strlen(ptr);

   subject.size_adjust(len);

   U_RETURN_STRING(subject);
}

UString UPKCS10::getSignable(X509_REQ* request)
{
   U_TRACE(1, "UPKCS10::getSignable(%p)", request)

   U_INTERNAL_ASSERT_POINTER(request)

   unsigned len = U_SYSCALL(i2d_X509_REQ_INFO, "%p,%p", request->req_info, 0);

   UString signable(len);

   unsigned char* data = (unsigned char*) signable.data();

   (void) U_SYSCALL(i2d_X509_REQ_INFO, "%p,%p", request->req_info, &data);

   // len = strlen(data);

   signable.size_adjust(len);

   U_RETURN_STRING(signable);
}

bool UPKCS10::verify(EVP_PKEY* publicKey) const
{
   U_TRACE(0, "UPKCS10::verify(%p)", publicKey)

   const char* alg = getSignatureAlgorithm().c_str();

   USignature sig(alg);

   sig.initVerify(publicKey);

   UString signable = getSignable();

   sig.update(signable);

   UString signature = getSignature();

   bool result = sig.verify(signature);

   U_RETURN(result);
}

UString UPKCS10::getEncoded(const char* format) const
{
   U_TRACE(0, "UPKCS10::getEncoded(%S)", format)

   U_INTERNAL_ASSERT_POINTER(request)

   if (U_STRNCMP(format, "DER") == 0)
      {
      unsigned len = U_SYSCALL(i2d_X509_REQ, "%p,%p", request, 0);

      UString encoding(len);

      unsigned char* temp = (unsigned char*) encoding.data();

      (void) U_SYSCALL(i2d_X509_REQ, "%p,%p", request, &temp);

      encoding.size_adjust(len);

      U_RETURN_STRING(encoding);
      }
   else if (U_STRNCMP(format, "PEM") == 0)
      {
      BIO* bio = (BIO*) U_SYSCALL(BIO_new, "%p", BIO_s_mem());

      (void) U_SYSCALL(PEM_write_bio_X509_REQ, "%p,%p", bio, request);

      UString encoding = UStringExt::BIOtoString(bio);

      U_RETURN_STRING(encoding);
      }

   U_RETURN_STRING(UString::getStringNull());
}

// STREAMS

U_EXPORT ostream& operator<<(ostream& os, const UPKCS10& c)
{
   U_TRACE(0+256,"UPKCS10::operator<<(%p,%p)", &os, &c)

   os.put('{');
   os.put(' ');

   BIO* bio = (BIO*) U_SYSCALL(BIO_new, "%p", BIO_s_mem());

   (void) X509_REQ_print(bio, c.request);

   UString text = UStringExt::BIOtoString(bio);

   (void) os.write(text.data(), text.size());

   os.put(' ');
   os.put('}');

   return os;
}

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UPKCS10::dump(bool reset) const
{
   *UObjectIO::os << "X509_REQ " << request;

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
