// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    mime_pkcs7.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/ssl/mime/mime_pkcs7.h>

UMimePKCS7::UMimePKCS7(const UString& _data)
{
   U_TRACE_REGISTER_OBJECT(0, UMimePKCS7, "%.*S", U_STRING_TO_TRACE(_data))

   // OpenSSL_add_all_algorithms(); // called in ULib_init()

   BIO* indata; // indata is the signed data if the content is not present in pkcs7 (that is it is detached)

   BIO* in = (BIO*) U_SYSCALL(BIO_new_mem_buf, "%p,%d", U_STRING_TO_PARAM(_data));

   // SMIME reader: handle multipart/signed and opaque signing. In multipart case the content is placed
   // in a memory BIO pointed to by "indata". In opaque this is set to NULL

   PKCS7* p7 = (PKCS7*) U_SYSCALL(SMIME_read_PKCS7, "%p,%p", in, &indata);

   (void) U_SYSCALL(BIO_free, "%p", in);

   pkcs7.set(p7, indata);

   content = pkcs7.getContent(&valid_content);
}

// STREAMS

U_EXPORT ostream& operator<<(ostream& os, const UMimePKCS7& mp7)
{
   U_TRACE(0, "UMimePKCS7::operator<<(%p,%p)", &os, &mp7)

   os.write(mp7.content.data(), mp7.content.size());

   return os;
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UMimePKCS7::dump(bool reset) const
{
   U_CHECK_MEMORY

   UMimeEntity::dump(false);

   *UObjectIO::os << "\n"
               << "valid_content             " << valid_content << '\n'
               << "pkcs7             (UPKCS7 " << (void*)&pkcs7 << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}
#endif
