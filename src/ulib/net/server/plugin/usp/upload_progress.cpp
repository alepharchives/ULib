// upload_progress.cpp
   
#include <ulib/net/server/usp_macro.h>
   
   
   
extern "C" {
extern U_EXPORT int runDynamicPage(UClientImage_Base* client_image);
       U_EXPORT int runDynamicPage(UClientImage_Base* client_image)
{
   U_TRACE(0, "::runDynamicPage(%p)", client_image)
   
   if (client_image == 0 || client_image == (UClientImage_Base*)-1 || client_image == (UClientImage_Base*)-2) U_RETURN(0);
   
   char usp_buffer[10 * 4096];
   uint32_t usp_sz, usp_num_args = 0;
   bool usp_as_service = (client_image == (UClientImage_Base*)-3);
   
   if (usp_as_service == false) (void) UClientImage_Base::wbuffer->append(
      U_CONSTANT_TO_PARAM("Content-Type: application/json\r\nPragma: no-cache\r\nExpires: Thu, 19 Nov 1981 08:52:00 GMT\r\nCache-Control: no-store, no-cache, must-revalidate, post-check=0, pre-check=0\r\n\r\n")
   );
   
   (void) UClientImage_Base::wbuffer->append(UHTTP::getUploadProgress());
   
   U_RETURN(usp_as_service ? U_NOT_FOUND : 0);
} }