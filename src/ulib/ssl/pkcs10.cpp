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
#include <ulib/utility/base64.h>
#include <ulib/utility/string_ext.h>

#include <openssl/pem.h>

X509_REQ* UPKCS10::readPKCS10(const UString& x, const char* format)
{
   U_TRACE(1, "UPKCS10::readPKCS10(%.*S,%S)", U_STRING_TO_TRACE(x), format)

   BIO* in;
   UString tmp        = x;
   X509_REQ* _request = 0;

   if (format == 0) format = (x.isBinary() ? "DER" : "PEM");

   if (U_STREQ(format, "PEM") &&
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

   _request = (X509_REQ*) (U_STREQ(format, "PEM") ? U_SYSCALL(PEM_read_bio_X509_REQ, "%p,%p,%p,%p", in, 0, 0, 0)
                                                  : U_SYSCALL(d2i_X509_REQ_bio,      "%p,%p",       in, 0));

   (void) U_SYSCALL(BIO_free, "%p", in);

   U_RETURN_POINTER(_request, X509_REQ);
}

UString UPKCS10::getSubject(X509_REQ* _request)
{
   U_TRACE(1, "UPKCS10::getSubject(%p)", _request)

   U_INTERNAL_ASSERT_POINTER(_request)

   X509_NAME* name = X509_REQ_get_subject_name(_request);
   unsigned len    = U_SYSCALL(i2d_X509_NAME, "%p,%p", name, 0);

   UString subject(len);
   char* ptr = subject.data();

   (void) U_SYSCALL(X509_NAME_oneline, "%p,%p,%d", name, ptr, subject.capacity());

   len = u__strlen(ptr, __PRETTY_FUNCTION__);

   subject.size_adjust(len);

   U_RETURN_STRING(subject);
}

UString UPKCS10::getSignable(X509_REQ* _request)
{
   U_TRACE(1, "UPKCS10::getSignable(%p)", _request)

   U_INTERNAL_ASSERT_POINTER(_request)

   unsigned len = U_SYSCALL(i2d_X509_REQ_INFO, "%p,%p", _request->req_info, 0);

   UString signable(len);

   unsigned char* data = (unsigned char*) signable.data();

   (void) U_SYSCALL(i2d_X509_REQ_INFO, "%p,%p", _request->req_info, &data);

   // len = u__strlen(data, __PRETTY_FUNCTION__);

   signable.size_adjust(len);

   U_RETURN_STRING(signable);
}

UString UPKCS10::getEncoded(const char* format) const
{
   U_TRACE(0, "UPKCS10::getEncoded(%S)", format)

   U_INTERNAL_ASSERT_POINTER(request)

   if (U_STREQ(format, "DER"))
      {
      unsigned len = U_SYSCALL(i2d_X509_REQ, "%p,%p", request, 0);

      UString encoding(len);

      unsigned char* temp = (unsigned char*) encoding.data();

      (void) U_SYSCALL(i2d_X509_REQ, "%p,%p", request, &temp);

      encoding.size_adjust(len);

      U_RETURN_STRING(encoding);
      }
   else if (U_STREQ(format, "PEM"))
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
