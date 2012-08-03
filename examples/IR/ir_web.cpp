// ir_web.cpp
   
#include <ulib/net/server/usp_macro.h>
   
#include "ir_session.h"
   
   static IR* ir;
   static Query* query;
   static UCrono* crono;
   static UString* footer;
   
   static void usp_init()
   {
   U_TRACE(5, "::usp_init()")
   
   U_INTERNAL_ASSERT_EQUALS(ir,0)
   U_INTERNAL_ASSERT_EQUALS(query,0)
   U_INTERNAL_ASSERT_EQUALS(crono,0)
   U_INTERNAL_ASSERT_EQUALS(footer,0)
   
   ir     = U_NEW(IR);
   query  = U_NEW(Query);
   crono  = U_NEW(UCrono);
   footer = U_NEW(UString(200U));
   
   UString cfg_index;
   
   if ((ir->loadFileConfig(cfg_index) &&
        ir->openCDB(false)) == false)
      {
      U_ERROR("usp_init() of servlet 'ir_web' failed...");
      }
   
   footer->snprintf("ver. %s, with %u documents and %u words.", ULIB_VERSION, cdb_names->size(), cdb_words->size());
   
   UHTTP::data_session = U_NEW(IRDataSession);
   }
   
   static void usp_end()
   {
   U_TRACE(5, "::usp_end()")
   
   U_INTERNAL_ASSERT_POINTER(ir)
   U_INTERNAL_ASSERT_POINTER(query)
   U_INTERNAL_ASSERT_POINTER(crono)
   U_INTERNAL_ASSERT_POINTER(footer)
   
   #ifdef DEBUG
   delete ir;
   delete query;
   delete crono;
   delete footer;
   #endif
   }  
   
extern "C" {
extern U_EXPORT int runDynamicPage(UClientImage_Base* client_image);
       U_EXPORT int runDynamicPage(UClientImage_Base* client_image)
{
   U_TRACE(0, "::runDynamicPage(%p)", client_image)
   
   if (client_image == 0)         { usp_init();  U_RETURN(0); }
   
   if (client_image == (void*)-1) {              U_RETURN(0); }
   
   if (client_image == (void*)-2) { usp_end();   U_RETURN(0); }
   
   uint32_t usp_sz;
   char usp_buffer[10 * 4096];
   bool usp_as_service = (client_image == (UClientImage_Base*)-3);
   
   union uudatasession {
      UDataSession*  po;
      IRDataSession* pe;
   };
   
   const char* ref     = "?ext=help";
   uint32_t num_args   = UHTTP::form_name_value->size() / 2;
   bool form_with_help = false;
   
   if (UHTTP::isHttpGET())
      {
      if (num_args == 1)
         {
         UString ext_val = USP_FORM_VALUE_FROM_NAME("ext");
   
         if (ext_val.equal(U_CONSTANT_TO_PARAM("help")))
            {
            ref            = "ir_web";
            form_with_help = true;
            }
         else
            {
            if (UHTTP::getDataSession(0,0) == false) return HTTP_BAD_REQUEST;
   
            UHTTP::num_page_cur = USP_FORM_VALUE(0).strtol();
   
            union uudatasession pd = { UHTTP::data_session };
   
            UHTTP::num_item_tot = pd.pe->vec->size();
            }
         }
      }
   else if (UHTTP::isHttpPOST())
      {
      UString phrase = USP_FORM_VALUE(0);
   
      if (num_args != 2 || phrase.empty()) return HTTP_BAD_REQUEST;
   
      if (UHTTP::getDataSession(0,0) == false) UHTTP::setSessionCookie(0);
   
      IR_SESSION.QUERY    = phrase;
      IR_SESSION.FOR_PAGE = UHTTP::num_item_for_page = USP_FORM_VALUE(1).strtol();
   
      query->clear();
   
      crono->start();
   
      query->run(IR_SESSION.QUERY.c_str());
   
      crono->stop();
   
      if ((UHTTP::num_item_tot = WeightWord::size()))
         {
         UHTTP::num_page_start = 1;
         UHTTP::num_page_end   = UHTTP::num_item_for_page;
   
         WeightWord::sortObjects();
   
         IR_SESSION.vec = WeightWord::vec;
                          WeightWord::vec = 0;
         }
   
      (void) snprintf(IR_SESSION.timebuf, sizeof(IR_SESSION.timebuf), "%f", crono->getTimeElapsedInSecond());
   
      UHTTP::num_page_cur = 1;
   
      UHTTP::putDataSession(0,0,0);
      }
   else
      {
      return HTTP_BAD_METHOD;
      }
   
   (void) UClientImage_Base::wbuffer->append(
      U_CONSTANT_TO_PARAM("<html>\n<head>\n  <title>ULib search engine: a full-text search system for communities</title>\n  <link title=\"Services\" rel=\"stylesheet\" href=\"/css/ir.css\" type=\"text/css\">\n</head>\n<body>\n  <div id=\"estform\" class=\"estform\">\n    <form action=\"ir_web\" method=\"post\" id=\"form_self\" name=\"form_self\">\n\n      <div class=\"form_navi\">\n        <a href=\"")
   );
   
   (void) UClientImage_Base::wbuffer->append(ref);
   
   (void) UClientImage_Base::wbuffer->append(
      U_CONSTANT_TO_PARAM("\" class=\"navilink\">help</a>\n      </div>\n\n      <div class=\"form_basic\">\n        <input type=\"text\" name=\"phrase\" value=\"\" size=\"80\" id=\"phrase\" class=\"text\" tabindex=\"1\" accesskey=\"0\">\n\t\t  <input type=\"submit\" value=\"Search\" id=\"search\" class=\"submit\" tabindex=\"2\" accesskey=\"1\">\n      </div>\n\n      <div class=\"form_extension\">\n        <select name=\"perpage\" id=\"perpage\" tabindex=\"3\">\n          <option value=\"10\" selected=\"selected\">10</option>\n          <option value=\"20\">20</option> \n          <option value=\"30\">30</option>\n          <option value=\"50\">50</option>\n          <option value=\"60\">60</option>\n          <option value=\"70\">70</option>\n          <option value=\"80\">80</option>\n          <option value=\"90\">90</option>\n          <option value=\"100\">100</option>\n        </select> per page\n      </div>\n    </form>\n  </div>\n\n")
   );
   
   if (form_with_help) {
   
   (void) UClientImage_Base::wbuffer->append(
      U_CONSTANT_TO_PARAM("\t<div class=\"help\">\n\t <h1 class=\"title\">Help</h1>\n\n\t <h2>What is This?</h2>\n\n\t <p>This is a full-text search system. You can search for documents including some specified words.</p>\n\n\t <h2>How to Use</h2>\n\n\t <p>Input search phrase into the field at the top of the page. For example, if you search for documents including \"computer\", input the\n\t following.</p>\n\t <pre>computer</pre>\n\n\t <p>If you search for documents including both of \"network\" and \"socket\", input the following.</p>\n\t <pre>network socket</pre>\n\n\t <p>It is the same as the following.</p>\n\t <pre>network AND socket</pre>\n\n\t <p>If you search for documents including \"network\" followed by \"socket\", input the following.</p>\n\t <pre>\"network socket\"</pre>\n\n\t <p>If you search for documents including one or both of \"network\" and \"socket\", input the following.</p>\n\t <pre>network OR socket</pre>\n\n\t <p>If you search for documents including \"network\" but without \"socket\", input the following.</p>\n\t <pre>network AND NOT socket</pre>\n\n\t <p>For more complex query, you can use \"<code>(</code>\". Note that the priority of \"<code>(</code>\" is higher than that of \"<code>AND</code>\",\n\t \"<code>OR</code>\" and \"<code>NOT</code>\". So, the following is to search for documents including one of \"F1\", \"F-1\", \"Formula One\", and including\n\t one of \"champion\" and \"victory\".</p>\n\t <pre>(F1 OR F-1 OR \"Formula One\") AND (champion OR victory)</pre>\n\n\t <h2>You can use DOS wildcard characters</h2>\n\n\t <p>If you search for documents including some words beginning with \"inter\", input the following.</p>\n\t <pre>inter*</pre>\n\n\t <p>If you search for documents including some words ending with \"sphere\", input the following.</p>\n\t <pre>*sphere</pre>\n\n\t <p>If you search for documents matching some words matching \"?n*able\" (unable, unavoidable, inevitable, ...), input the following.</p>\n\t <pre>?n*able</pre>\n\n\t <h2>Other Faculties</h2>\n\n\t <p>\"<code>[...] per page</code>\" specifies the number of shown documents per page. If documents over one page correspond, you can move to another\n\t page via anchors of \"<code>PREV</code>\" and \"<code>NEXT</code>\" at the bottom of the page.</p>\n\n\t <h2>Information</h2>\n\n\t <p>See <a href=\"http://www.unirel.com/\">the project site</a> for more detail.</p>\n\t</div>\n\n")
   );
   
   } else {
      if (num_args == 0) {
   
   (void) UClientImage_Base::wbuffer->append(
      U_CONSTANT_TO_PARAM("\t<div class=\"logo\">\n\t\t<h1 class=\"title\">ULib search engine</h1>\n\t\t<div class=\"caption\">a full-text search system for communities</div>\n\t</div>\n\n")
   );
   
   } else {
   
         UString link_paginazione = UHTTP::getLinkPagination();
   
         USP_PRINTF("<div id=\"estresult\" class=\"estresult\">\n"
                    "  <div class=\"resinfo\">\n"
                    "  Results of <strong>%u</strong> - <strong>%u</strong> of about <strong>%u</strong> for <strong>%s</strong> (%s sec.)\n"
                    "  </div>\n",
                    UHTTP::num_page_start, UHTTP::num_page_end, UHTTP::num_item_tot, IR_SESSION.QUERY.data(), IR_SESSION.timebuf);
   
         if (UHTTP::num_item_tot == 0) {
   
   (void) UClientImage_Base::wbuffer->append(
      U_CONSTANT_TO_PARAM("\t\t\t<p class=\"note\">Your search did not match any documents.</p>\n")
   );
   
   } else {
   
            UString DOC, SNIPPET_DOC(U_CAPACITY), _basename, filename, pathname1(U_CAPACITY), pathname2(U_CAPACITY);
   
            for (uint32_t i = UHTTP::num_page_start-1; i < UHTTP::num_page_end; ++i)
               {
               filename  = (*IR_SESSION.vec)[i]->filename;
               _basename = UStringExt::basename(filename);
   
               pathname1.snprintf(  "/doc/%.*s", U_STRING_TO_TRACE(filename));
               pathname2.snprintf("%w/doc/%.*s", U_STRING_TO_TRACE(filename));
   
               DOC = UFile::contentOf(pathname2);
   
               UXMLEscape::encode(DOC, SNIPPET_DOC);
   
               USP_PRINTF("<dl class=\"doc\"\n"
                          "  <dt><a href=\"%s\" class=\"doc_title\">%.*s</a></dt>\n"
                          "  <dd class=\"doc_text\">%s <code class=\"delim\">...</code></dd>\n"
                          "  <dd class=\"doc_navi\"><span class=\"doc_link\">file://%s</span></dd>\n"
                          "</dl>\n",
                          pathname1.data(), U_STRING_TO_TRACE(_basename), SNIPPET_DOC.data(), pathname2.data());
               }
            }
   
         USP_PRINTF("<div class=\"paging\">%.*s</div></div>\n", U_STRING_TO_TRACE(link_paginazione));
         }
   }
   
   (void) UClientImage_Base::wbuffer->append(
      U_CONSTANT_TO_PARAM("  <div id=\"estinfo\" class=\"estinfo\">\n    Powered by <a href=\"http://www.unirel.com/\">ULib search engine</a> ")
   );
   
   (void) UClientImage_Base::wbuffer->append(*footer);
   
   (void) UClientImage_Base::wbuffer->append(
      U_CONSTANT_TO_PARAM("  </div>\n</body>\n</html>")
   );
   
   U_RETURN(usp_as_service ? 0 : 200);
} }