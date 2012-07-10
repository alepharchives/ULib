// upload_progress.cpp
   
#include <ulib/net/server/usp_macro.h>
   
static void usp_init()
   {
   U_TRACE(5, "::usp_init()")
   
   UHTTP::ptr_upload_progress = (UHTTP::upload_progress*) UServer_Base::getPointerToDataShare(UHTTP::ptr_upload_progress);
   
   U_INTERNAL_ASSERT_EQUALS(UHTTP::ptr_upload_progress->byte_read,0)
   }  
   
extern "C" {
extern U_EXPORT int runDynamicPage(UClientImage_Base* client_image);
       U_EXPORT int runDynamicPage(UClientImage_Base* client_image)
{
   U_TRACE(0, "::runDynamicPage(%p)", client_image)
   
   if (client_image == 0)         { usp_init();  U_RETURN(0); }
   
   if (client_image == (void*)-1) {              U_RETURN(0); }
   
   if (client_image == (void*)-2) {              U_RETURN(0); }
   
   uint32_t usp_sz;
   char usp_buffer[10 * 4096];
   bool usp_as_service = (client_image == (UClientImage_Base*)-3);
   
   if (usp_as_service == false) (void) UClientImage_Base::wbuffer->append(
      U_CONSTANT_TO_PARAM("Content-Type: application/json\r\nCache Control: max-age=0\r\nExpires: Thu, 19 Nov 1981 08:52:00 GMT\r\n\r\n")
   );
   
   (void) UClientImage_Base::wbuffer->append(UHTTP::getUploadProgress());
   
   U_RETURN(usp_as_service ? U_NOT_FOUND : 0);
} }