#include <ulib/all.h>

#define U_DYNAMIC_PAGE_APPEND(string)              (void)UClientImage_Base::wbuffer->append(string)

#define U_DYNAMIC_PAGE_OUTPUT(fmt,args...)          (UClientImage_Base::_buffer->snprintf(fmt , ##args),UClientImage_Base::wbuffer->append(*UClientImage_Base::_buffer))

#define U_DYNAMIC_PAGE_OUTPUT_ENCODED(fmt,args...) (UClientImage_Base::_buffer->snprintf(fmt , ##args),UXMLEscape::encode(*UClientImage_Base::_buffer,*UClientImage_Base::_encoded),UClientImage_Base::wbuffer->append(*UClientImage_Base::_encoded))

#define U_DYNAMIC_PAGE_GET_FORM_VALUE(n)                UHTTP::getFormValue(*UClientImage_Base::_value, n)

#define U_DYNAMIC_PAGE_OUTPUT_FORM_VALUE(fmt,n)         (U_DYNAMIC_PAGE_GET_FORM_VALUE(n),U_DYNAMIC_PAGE_OUTPUT(fmt,U_STRING_TO_TRACE(*UClientImage_Base::_value)))
#define U_DYNAMIC_PAGE_OUTPUT_ENCODED_FORM_VALUE(fmt,n) (U_DYNAMIC_PAGE_GET_FORM_VALUE(n),U_DYNAMIC_PAGE_OUTPUT_ENCODED(fmt,U_STRING_TO_TRACE(*UClientImage_Base::_value)))


extern "C" {
extern U_EXPORT void runDynamicPage(UClientImage_Base* client_image);
       U_EXPORT void runDynamicPage(UClientImage_Base* client_image)
{
  U_TRACE(0, "::runDynamicPage(%p)", client_image)

  if (client_image == 0 || client_image == (UClientImage_Base*)-1 || client_image == (UClientImage_Base*)-2) return;

  U_INTERNAL_ASSERT_POINTER(UClientImage_Base::_value)
  U_INTERNAL_ASSERT_POINTER(UClientImage_Base::_buffer)
  U_INTERNAL_ASSERT_POINTER(UClientImage_Base::request)
  U_INTERNAL_ASSERT_POINTER(UClientImage_Base::rbuffer)
  U_INTERNAL_ASSERT_POINTER(UClientImage_Base::wbuffer)
  U_INTERNAL_ASSERT_POINTER(UClientImage_Base::_encoded)
  U_INTERNAL_ASSERT_EQUALS( UClientImage_Base::pClientImage, client_image)

(void) UClientImage_Base::wbuffer->append(U_CONSTANT_TO_PARAM("Content-Type: application/json\r\nPragma: no-cache\r\nExpires: Thu, 19 Nov 1981 08:52:00 GMT\r\nCache-Control: no-store, no-cache, must-revalidate, post-check=0, pre-check=0\r\n\r\n"));
U_DYNAMIC_PAGE_APPEND(UHTTP::getUploadProgress());
} }
