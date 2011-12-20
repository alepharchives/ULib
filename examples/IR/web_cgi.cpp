// web_cgi.cpp

#include <ulib/cache.h>
#include <ulib/debug/crono.h>
#include <ulib/net/server/server.h>
#include <ulib/utility/string_ext.h>
#include <ulib/utility/xml_escape.h>

#include "ir_session.h"

#define FORM_PART1            FORM_PART[0]
#define FORM_PART2            FORM_PART[1]
#define FORM_PART3            FORM_PART[2]

#define SCRIPT_NAME           "seek"
#define IR_DIRECTORY          "/IR/doc"                                                      // location of index db (start with / for skip /usp)...
#define DOC_DIRECTORY         "/usr/src/ULib-" ULIB_VERSION "/tests/examples" IR_DIRECTORY   // location of docs indexed...
#define DB_SESSION            "IR/WEB/result/session"                                        // location of session db...
#define FORM_FILE_DIR_DEFAULT "IR/WEB/form/en"                                               // default directory of form template...
#define SESSION_SIZE          (16 * 1024)                                                    // num entry of session storage...

extern "C" {
extern U_EXPORT int runDynamicPage(UClientImage_Base* client_image);

// ENV
static IR* ir;
static Query* query;
static UCache* cache;
static UCrono* crono;
static UString* footer;
static UString* cookie;
static URDB* rdb_session;
static UString* set_cookie;
static IRSession* session;
static char FORM_FILE_DIR[256];
static UHashMap<IRSession*>* tbl_session;
static const char* FORM_PART[3] = { 0, 0, ULIB_VERSION };

// paginazione
static UString* link_paginazione;
static uint32_t NUM_START, NUM_END, NUM_DOC, pagina_corrente;

static UString* output;

static void set_ENV(const UString& buffer)
{
   U_TRACE(5, "::set_ENV(%.*S)", U_STRING_TO_TRACE(buffer))

   U_INTERNAL_ASSERT_POINTER(output)

   static char  old_accept_language[3];
    const char*     accept_language = (u_http_info.accept_language_len ? u_http_info.accept_language : "en");

   if (memcmp(accept_language, old_accept_language, 2))
      {
      (void) snprintf(FORM_FILE_DIR, sizeof(FORM_FILE_DIR), "IR/WEB/form/%.2s", (char*) u_memcpy(old_accept_language, accept_language, 2));

      if (UFile::access(FORM_FILE_DIR) == false) (void) u_strcpy(FORM_FILE_DIR, FORM_FILE_DIR_DEFAULT);
      }

   *cookie = UHTTP::getHTTPCookie();

   U_INTERNAL_DUMP("cookie = %.*S", U_STRING_TO_TRACE(*cookie))

   output->setBuffer(8192U);
}

static void view_form()
{
   U_TRACE(5, "::view_form()")

   U_INTERNAL_ASSERT_POINTER(FORM_PART1)
   U_INTERNAL_ASSERT_POINTER(FORM_PART2)

   output->snprintf(cache->contentOf("%s/%s.tmpl", FORM_FILE_DIR, SCRIPT_NAME).data(), "usp", FORM_PART1, FORM_PART2, FORM_PART3);

   FORM_PART1 = FORM_PART2 = 0;
}

static void view_form_without_help()
{
   U_TRACE(5, "::view_form_without_help()")

   UString form_part2;

   FORM_PART1 = "?ext=help";

   if (FORM_PART2 == 0)
      {
      form_part2 = cache->contentOf("%s/%s.top", FORM_FILE_DIR, SCRIPT_NAME);

      FORM_PART2 = form_part2.data();
      }

   view_form();
}

static void view_form_with_help()
{
   U_TRACE(5, "::view_form_with_help()")

   UString form_part2;

   FORM_PART1 = SCRIPT_NAME ".usp";

   if (FORM_PART2 == 0)
      {
      form_part2 = cache->contentOf("%s/%s.hlp", FORM_FILE_DIR, SCRIPT_NAME);

      FORM_PART2 = form_part2.data();
      }

   view_form();
}

static void view_page()
{
   U_TRACE(5, "::view_page()")

   // header

   UString RESULT_TOP(U_CAPACITY), RESULT_DOC(U_CAPACITY), RESULT_PAG(U_CAPACITY);

   RESULT_TOP.snprintf(cache->contentOf("%s/result.top", FORM_FILE_DIR).data(), NUM_START, NUM_END, NUM_DOC, session->QUERY.data(), session->TIME);

   // documents

   if (NUM_DOC == 0) RESULT_DOC.snprintf("<p class=\"note\">Your search did not match any documents.</p>");
   else
      {
      /* result.doc
       * -----------------------------------------------------------------
       * <dl class="doc">
       * <dt><a href="%s" class="doc_title">%s</a></dt> 
       * <dd class="doc_text">%s <code class="delim">...</code></dd> 
       * <dd class="doc_navi"><span class="doc_link">file://%s</span></dd>
       * </dl>
       * -----------------------------------------------------------------
       */

      UString DOC, SNIPPET_DOC(U_CAPACITY), ADD(U_CAPACITY), basename, filename, pathname1(U_CAPACITY), pathname2(U_CAPACITY),
              RESULT_DOC_FILE = cache->contentOf("%s/result.doc", FORM_FILE_DIR);

      for (uint32_t i = NUM_START-1; i < NUM_END; ++i)
         {
         filename = (*(session->vec))[i]->filename;
         basename = UStringExt::basename(filename);

         pathname1.snprintf("%s/%.*s",  IR_DIRECTORY, U_STRING_TO_TRACE(filename));
         pathname2.snprintf("%s/%.*s", DOC_DIRECTORY, U_STRING_TO_TRACE(filename));

         DOC = UFile::contentOf(pathname2);

         UXMLEscape::encode(DOC, SNIPPET_DOC);

         ADD.snprintf(RESULT_DOC_FILE.data(), pathname1.data(), basename.c_str(), SNIPPET_DOC.c_str(), pathname2.data());

         (void) RESULT_DOC.append(ADD);
         }
      }

   // link page

   RESULT_PAG.snprintf(cache->contentOf("%s/result.pag", FORM_FILE_DIR).data(), link_paginazione->c_str());

   (void) RESULT_TOP.append(RESULT_DOC);
   (void) RESULT_TOP.append(RESULT_PAG);

   FORM_PART2 = RESULT_TOP.c_str();
   FORM_PART3 = footer->data();

   output->setBuffer(RESULT_TOP.size() + 4096);

   view_form_without_help();
}

// funzioni che creano i link alle pagine dei risultati
// ----------------------------------------------------

static void crea_link(uint32_t n)
{
   U_TRACE(5, "::crea_link(%u)", n)

   UString ADD(U_CAPACITY);

   if (pagina_corrente == n) ADD.snprintf("<span class=\"pnow\">%u</span>", n);
   else                      ADD.snprintf("<a href=\"?page=%u\" class=\"pnum\">%u</a>", n, n);

   (void) link_paginazione->append(ADD);
          link_paginazione->push_back(' ');
}

static void set_paginazione()
{
   U_TRACE(5, "::set_paginazione()")

   U_INTERNAL_ASSERT_POINTER(session)
   U_INTERNAL_ASSERT_MAJOR(session->FOR_PAGE, 0)

   link_paginazione->setBuffer(U_CAPACITY);

   if (NUM_DOC <= session->FOR_PAGE)
      {
      NUM_END    =  NUM_DOC;
      NUM_START  = (NUM_DOC > 0);

      (void) link_paginazione->assign(U_CONSTANT_TO_PARAM("<span class=\"void\">PREV</span><span class=\"void\">NEXT</span>"));

      return;
      }

   uint32_t pagina_precedente = pagina_corrente - 1,
            pagina_successiva = pagina_corrente + 1,
            tot_pagine        = (NUM_DOC / session->FOR_PAGE);

   if ((NUM_DOC % session->FOR_PAGE) != 0) ++tot_pagine;

   uint32_t    ultima_pagina =    tot_pagine - 1,
            penultima_pagina = ultima_pagina - 1;

   // link alla pagina precedente

   if (pagina_corrente == 1)
      {
      NUM_START = 1;

      (void) link_paginazione->assign(U_CONSTANT_TO_PARAM("<span class=\"void\">PREV</span> "));
      }
   else
      {
      NUM_START = 1 + (pagina_precedente * session->FOR_PAGE);

      link_paginazione->snprintf("<a href=\"?page=%u\" class=\"pnum\">PREV</a> ", pagina_precedente);
      }

   // mostriamo sempre il link alla prima pagina

   crea_link(1);

   // se il prossimo link non è alla seconda pagina aggiungo dei puntini ... oppure la sola pagina mancante

   if (pagina_precedente > 2)
      {
      if (pagina_precedente == 3) crea_link(2);
      else                 (void) link_paginazione->append(U_CONSTANT_TO_PARAM(" ... "));
      }

   // creo i link alla pagina corrente ed a quelle ad essa vicine

   for (uint32_t i = pagina_precedente; i <= pagina_successiva; ++i)
      {
      // se tra quelle vicine c'è la prima pagina (già riportata)

      if (i < 2) continue;

      // se tra quelle vicine c'è l'ultima pagina (che mostrerò con le prossime istruzioni)

      if (i > ultima_pagina) continue;

      crea_link(i);
      }

   // se il precedente link non era alla penultima pagina aggiungo dei puntini ... oppure la sola pagina mancante

   if (pagina_successiva < ultima_pagina)
      {
      if (pagina_successiva == penultima_pagina) crea_link(ultima_pagina);
      else                                (void) link_paginazione->append(U_CONSTANT_TO_PARAM(" ... "));
      }

   // mostriamo il link all'ultima pagina se questa non coincide con la prima

   if (tot_pagine != 1) crea_link(tot_pagine);

   // link alla pagina successiva

   if (pagina_corrente == tot_pagine)
      {
      NUM_END = NUM_DOC;

      (void) link_paginazione->append(U_CONSTANT_TO_PARAM("<span class=\"void\">NEXT</span>"));
      }
   else
      {
      NUM_END = NUM_START + session->FOR_PAGE - 1;

      link_paginazione->snprintf_add("<a href=\"?page=%u\" class=\"pnum\">NEXT</a>", pagina_successiva);
      }
}
// ------------------------------------------------------

static void load_value_session()
{
   U_TRACE(5, "::load_value_session()")

   if (UServer_Base::preforked_num_kids == 0) session = (*tbl_session)[*cookie];
   else
      {
      UString data = (*rdb_session)[*cookie];

      if (data.empty() == false)
         {
         session = U_NEW(IRSession(data));

         U_ASSERT_EQUALS(data, session->toString())
         }
      }
}

static void save_value_session()
{
   U_TRACE(5, "::save_value_session()")

   if (cookie->empty())
      {
      // set session cookie

      static uint32_t counter;

      UString ip_client = UHTTP::getRemoteIP();

      // REQ: [ data expire path domain secure HttpOnly ]
      // ----------------------------------------------------------------------------------------------------------------------------
      // string -- Data to put into the cookie        -- must
      // int    -- Lifetime of the cookie in HOURS    -- must (0 -> valid until browser exit)
      // string -- Path where the cookie can be used  --  opt
      // string -- Domain which can read the cookie   --  opt
      // bool   -- Secure mode                        --  opt
      // bool   -- Only allow HTTP usage              --  opt
      // ----------------------------------------------------------------------------------------------------------------------------
      // RET: Set-Cookie: ulib_sid=data&expire&HMAC-MD5(data&expire); expires=expire(GMT); path=path; domain=domain; secure; HttpOnly

          cookie->setBuffer(100U);
          cookie->snprintf("%.*s_%P_%u", U_STRING_TO_TRACE(ip_client), counter++);         // session key...
      set_cookie->snprintf("Set-Cookie: TODO[ %.*s 24 ]\r\n", U_STRING_TO_TRACE(*cookie)); // like as shell script...
      }

   if (UServer_Base::preforked_num_kids == 0) tbl_session->insert(*cookie, session);
   else
      {
      UString data = session->toString();

      if (data.empty() == false)
         {
         int result = rdb_session->store(*cookie, data, RDB_REPLACE);

         if (result) U_SRV_LOG("error '%d' on store session data...", result);
         }

      delete session;
      }

   session = 0;
}

static void execute_query(UClientImage_Base* client_image)
{
   U_TRACE(5, "::execute_query(%p)", client_image)

   // run query

   query->clear();

   crono->start();

   query->run(session->QUERY.c_str());

   crono->stop();

   NUM_DOC = WeightWord::size();

   if (NUM_DOC)
      {
      NUM_START = 1;
      NUM_END   = session->FOR_PAGE;

      WeightWord::sortObjects();

      session->vec = WeightWord::vec;
                     WeightWord::vec = 0;
      }

   (void) snprintf(session->TIME, sizeof(session->TIME), "%f", crono->getTimeElapsedInSecond());

   pagina_corrente = 1;

   set_paginazione();

   view_page();
}

static void view_page_result()
{
   U_TRACE(5, "::view_page_result()")

   set_paginazione();

   view_page();
}

// (Server-wide hooks)...

static void usp_init()
{
   U_TRACE(5, "::usp_init()")

   U_INTERNAL_ASSERT_EQUALS(ir, 0)

   ir = U_NEW(IR);

   UString cfg_index;

   if ((ir->loadFileConfig(cfg_index) && ir->openCDB(false)) == false) U_ERROR("error on usp_init query application...");

   query            = U_NEW(Query);
   cache            = U_NEW(UCache);
   crono            = U_NEW(UCrono);
   cookie           = U_NEW(UString);
   output           = U_NEW(UString);
   footer           = U_NEW(UString(200U));
   set_cookie       = U_NEW(UString(80U));
   link_paginazione = U_NEW(UString);

   U_INTERNAL_DUMP("UServer_Base::preforked_num_kids = %u", UServer_Base::preforked_num_kids)

   if (UServer_Base::preforked_num_kids == 0)
      {
      tbl_session = U_NEW(UHashMap<IRSession*>);

      tbl_session->allocate(U_GET_NEXT_PRIME_NUMBER(SESSION_SIZE));
      }
   else
      {
      rdb_session = U_NEW(URDB(U_STRING_FROM_CONSTANT(DB_SESSION), false));

      // NB: the old sessions are automatically invalid...
      // (UServer generate the crypto key at startup...) 

      if (rdb_session->open(SESSION_SIZE, true) == false) U_ERROR("error on open session db %S...", DB_SESSION);
      }

   footer->snprintf("%s, with %u documents and %u words.", ULIB_VERSION, cdb_names->size(), cdb_words->size());

   if (cache->open(U_STRING_FROM_CONSTANT("IR/WEB/form/cache.frm"), 32 * 1024) == false) cache->loadContentOf(U_STRING_FROM_CONSTANT(FORM_FILE_DIR_DEFAULT));
}

// (Connection-wide hooks)...

static void usp_reset()
{
   U_TRACE(5, "::usp_reset()")

   U_INTERNAL_ASSERT_POINTER(output)
   U_INTERNAL_ASSERT_POINTER(link_paginazione)

             output->clear();
   link_paginazione->clear();
}

static void usp_end()
{
   U_TRACE(5, "::usp_end()")

   U_INTERNAL_ASSERT_POINTER(ir)

   delete ir;

   if (query)
      {
      delete query;
      delete cache;
      delete crono;
      delete footer;
      delete cookie;
      delete output;
      delete set_cookie;
      delete link_paginazione;

      if (UServer_Base::preforked_num_kids == 0)
         {
         tbl_session->clear();
         tbl_session->deallocate();

         delete tbl_session;
         }
      else
         {
         rdb_session->close();

         delete rdb_session;

         if (session) delete session;
         }
      }
}

U_EXPORT int runDynamicPage(UClientImage_Base* client_image)
{
   U_TRACE(5, "::runDynamicPage(%p)", client_image)

        if (client_image ==         0) { usp_init();  return 0; } // usp_init  (    Server-wide hooks)...
   else if (client_image == (void*)-1) { usp_reset(); return 0; } // usp_reset (Connection-wide hooks)...
   else if (client_image == (void*)-2) { usp_end();   return 0; } // usp_end

   set_ENV(*client_image->request);

   uint32_t n = UHTTP::form_name_value->size() / 2;

   U_INTERNAL_DUMP("n = %u", n)

   if (UHTTP::isHttpGET())
      {
      if (n == 0)
         {
         view_form_without_help();
         }
      else if (n == 1)
         {
         if ((*UHTTP::form_name_value)[1].equal(U_CONSTANT_TO_PARAM("help")))
            {
            view_form_with_help();
            }
         else
            {
            pagina_corrente = (*UHTTP::form_name_value)[1].strtol();

            load_value_session();

            if (session == 0) goto bad_request;

            view_page_result();
            }
         }
      }
   else if (UHTTP::isHttpPOST() && n == 2)
      {
      UString QUERY = (*UHTTP::form_name_value)[1];

      if (QUERY.empty()) goto bad_request;

      session = U_NEW(IRSession(QUERY, (*UHTTP::form_name_value)[3].strtol()));

      execute_query(client_image);

      save_value_session();
      }

   if (output->empty() == false)
      {
      UClientImage_Base::checkCookie();

      (void) client_image->wbuffer->append(U_CONSTANT_TO_PARAM("Content-Type: " U_CTYPE_HTML "\r\n\r\n"));
      (void) client_image->wbuffer->append(*output);

      U_RETURN(0);
      }

bad_request:
   UHTTP::setHTTPBadRequest();

   U_RETURN(0);
}
}
