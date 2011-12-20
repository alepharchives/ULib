// ir_web.cpp
   
#include <ulib/net/server/usp_macro.h>
   
#include "ir_session.h"
   
extern "C" {
extern U_EXPORT int runDynamicPage(UClientImage_Base* client_image);
       U_EXPORT int runDynamicPage(UClientImage_Base* client_image)
{
   U_TRACE(0, "::runDynamicPage(%p)", client_image)
   
   if (client_image == 0 || client_image == (UClientImage_Base*)-1 || client_image == (UClientImage_Base*)-2) U_RETURN(0);
   
   UClientImage_Base::checkCookie();
   UClientImage_Base::wbuffer->snprintf("Content-Type: " U_CTYPE_HTML "\r\n\r\n");
   
   (void) UClientImage_Base::wbuffer->append(
      U_CONSTANT_TO_PARAM("<html>\n<head>\n  <title>ULib search engine: a full-text search system for communities</title>\n  <link title=\"Services\" rel=\"stylesheet\" href=\"/IR/WEB/css/ir.css\" type=\"text/css\">\n</head>\n<body>\n  <div id=\"estform\" class=\"estform\">\n    <form action=\"seek...\" method=\"post\" id=\"form_self\" name=\"form_self\">\n      <div class=\"form_navi\">\n        <a href=\"...\" class=\"navilink\">help</a>\n      </div>\n\n      <div class=\"form_basic\">\n        <input type=\"text\" name=\"phrase\" value=\"\" size=\"80\" id=\"phrase\" class=\"text\" tabindex=\"1\" accesskey=\"0\">\n\t\t  <input type=\"submit\" value=\"Search\" id=\"search\" class=\"submit\" tabindex=\"2\" accesskey=\"1\">\n      </div>\n\n      <div class=\"form_extension\">\n        <select name=\"perpage\" id=\"perpage\" tabindex=\"3\">\n          <option value=\"10\" selected=\"selected\">10</option>\n          <option value=\"20\">20</option> \n          <option value=\"30\">30</option>\n          <option value=\"50\">50</option>\n          <option value=\"60\">60</option>\n          <option value=\"70\">70</option>\n          <option value=\"80\">80</option>\n          <option value=\"90\">90</option>\n          <option value=\"100\">100</option>\n        </select> per page\n      </div>\n    </form>\n  </div>...\n  <div id=\"estinfo\" class=\"estinfo\">\n    Powered by <a href=\"http://www.unirel.com/\">ULib search engine</a> ...\n  </div>\n</body>\n</html>")
   );
   
   U_RETURN(0);
} }