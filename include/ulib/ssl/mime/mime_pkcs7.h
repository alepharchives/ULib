// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    mime_pkcs7.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef U_MIME_PKCS7_H
#define U_MIME_PKCS7_H 1

#include <ulib/ssl/pkcs7.h>
#include <ulib/mime/entity.h>

class U_EXPORT UMimePKCS7 : public UMimeEntity {
public:

   // COSTRUTTORI

   UMimePKCS7(const UString& data);

   UMimePKCS7(UMimeEntity& item) : UMimeEntity(item), pkcs7(content, "DER")
      {
      U_TRACE_REGISTER_OBJECT(0, UMimePKCS7, "%p", &item)

      U_ASSERT(UMimeEntity::isPKCS7())

      if (pkcs7.isValid()) content = pkcs7.getContent(&valid_content);
      }

   ~UMimePKCS7()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UMimePKCS7)
      }

   // VARIE

   UPKCS7& getPKCS7() { return pkcs7; }

   bool isValid() const
      {
      U_TRACE(0, "UMimePKCS7::isValid()")

      bool result = (pkcs7.isValid() && valid_content);

      U_RETURN(result);
      }

   // STREAM

   friend U_EXPORT ostream& operator<<(ostream& os, const UMimePKCS7& mp7);

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   UPKCS7 pkcs7;
   bool valid_content;

private:
   UMimePKCS7(const UMimePKCS7&) : UMimeEntity() {}
   UMimePKCS7& operator=(const UMimePKCS7&)      { return *this; }
};

#endif
