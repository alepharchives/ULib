// web_cgi.cpp

#include <ulib/cache.h>
#include <ulib/debug/crono.h>
#include <ulib/net/server/server.h>
#include <ulib/internal/objectIO.h>
#include <ulib/utility/xml_escape.h>

#include "cquery.h"

extern "C" {
extern U_EXPORT void runDynamicPage(UClientImage_Base* client_image);

#define FORM_PART1            FORM_PART[0]
#define FORM_PART2            FORM_PART[1]
#define FORM_PART3            FORM_PART[2]

#define SCRIPT_NAME           "seek"
#define IR_DIRECTORY          "/IR/doc"                                                // location of index db (start with / for skip /usp)...
#define DOC_DIRECTORY         "/usr/src/ULib-" VERSION "/tests/examples" IR_DIRECTORY  // location of docs indexed...
#define DB_SESSION            "IR/WEB/result/session"                                  // location of session db...
#define FORM_FILE_DIR_DEFAULT "IR/WEB/form/en"                                         // default directory of form template...
#define SESSION_SIZE          (16 * 1024)                                              // num entry of session storage...

class DataSession {
public:

   // Check Memory
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   // COSTRUTTORE

    DataSession(const UString& _QUERY, uint32_t _FOR_PAGE) : QUERY(_QUERY), FOR_PAGE(_FOR_PAGE)
      {
      U_TRACE_REGISTER_OBJECT(5, DataSession, "%.*S,%u", U_STRING_TO_TRACE(QUERY), FOR_PAGE)

      vec     = 0;
      TIME[0] = 0;
      }

    DataSession(const UString& data)
      {
      U_TRACE_REGISTER_OBJECT(5, DataSession, "%.*S", U_STRING_TO_TRACE(data))

      istrstream is(data.data(), data.size());

      is >> *this;
      }

   ~DataSession()
      {
      U_TRACE_UNREGISTER_OBJECT(5, DataSession)

      if (vec) delete vec;
      }

   // SERVICES

   UString toString()
      {
      U_TRACE(5, "DataSession::toString()")

      if (WeightWord::size_streambuf_vec)
         {
         UString buffer(WeightWord::size_streambuf_vec);

         uint32_t pcount = UObject2String<DataSession>(*this, buffer.data(), buffer.capacity());

         buffer.size_adjust(pcount);

         U_RETURN_STRING(buffer);
         }

      U_RETURN_STRING(UString::getStringNull());
      }

   // STREAM

   friend istream& operator>>(istream& is, DataSession& d)
      {
      U_TRACE(5, "DataSession::operator>>(%p,%p)", &is, &d)

      U_INTERNAL_ASSERT_EQUALS(is.peek(), '{')

      is.get(); // skip '{'

      is >> d.TIME
         >> d.FOR_PAGE;

      is.get(); // skip ' '

      d.QUERY.get(is);

      uint32_t vsize;
         is >> vsize;

      is.get(); // skip ' '

      // load filenames

      UVector<UString> vtmp(vsize);
                 is >> vtmp;

      WeightWord::clear();
      WeightWord::allocVector(vsize);

      UPosting::word_freq = 0;

      for (uint32_t i = 0, sz = vtmp.size(); i < sz; ++i)
         {
         *UPosting::filename = vtmp[i];

         WeightWord::push();
         }

      d.vec = WeightWord::vec;
              WeightWord::vec = 0;

      return is;
      }

   friend ostream& operator<<(ostream& os, const DataSession& d)
      {
      U_TRACE(5, "DataSession::operator<<(%p,%p)", &os, &d)

      os.put('{');
      os.put(' ');
      os << d.TIME;
      os.put(' ');
      os << d.FOR_PAGE;
      os.put(' ');
      d.QUERY.write(os);
      os.put(' ');
      os << d.vec->size();
      os.put(' ');
      os << *d.vec;
      os.put(' ');
      os.put('}');

      return os;
      }

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

   const char* dump(bool reset) const
      {
      *UObjectIO::os << "TIME                         ";

      char buffer[20];

      UObjectIO::os->write(buffer, u_snprintf(buffer, sizeof(buffer), "%S", TIME));

      *UObjectIO::os << '\n'
                     << "FOR_PAGE                     " << FOR_PAGE      << '\n'
                     << "QUERY (UString               " << (void*)&QUERY << ")\n"
                     << "vec   (UVector<WeightWord*>) " << (void*)vec    << ')';

      if (reset)
         {
         UObjectIO::output();

         return UObjectIO::buffer_output;
         }

      return 0;
      }
#endif

public:
   UString QUERY;
   UVector<WeightWord*>* vec;
   uint32_t FOR_PAGE;
   char TIME[9];
};

// ENV
static IR* ir;
static Query* query;
static UCache* cache;
static UCrono* crono;
static UString* footer;
static UString* cookie;
static URDB* rdb_session;
static UString* set_cookie;
static DataSession* session;
static char FORM_FILE_DIR[256];
static UHashMap<DataSession*>* tbl_session;
static const char* FORM_PART[3] = { 0, 0, VERSION };

// paginazione
static UString* link_paginazione;
static uint32_t NUM_START, NUM_END, NUM_DOC, pagina_corrente;

static UString* output;

static void set_ENV(const UString& rbuffer)
{
   U_TRACE(5, "::set_ENV(%.*S)", U_STRING_TO_TRACE(rbuffer))

   U_INTERNAL_ASSERT_POINTER(output)

   static char  old_accept_language[3];
    const char*     accept_language = UHTTP::getAcceptLanguage();

   if (memcmp(accept_language, old_accept_language, 2))
      {
      (void) snprintf(FORM_FILE_DIR, sizeof(FORM_FILE_DIR), "IR/WEB/form/%.2s", (char*) memcpy(old_accept_language, accept_language, 2));

      if (UFile::access(FORM_FILE_DIR) == false) (void) strcpy(FORM_FILE_DIR, FORM_FILE_DIR_DEFAULT);
      }

   *cookie = UHTTP::getHTTPCookie(true);

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

   if (NUM_DOC == 0) RESULT_DOC.snprintf("<p class=\"note\">Your search did not match any documents.</p>", 0);
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
         session = U_NEW(DataSession(data));

         U_ASSERT_EQUALS(data, session->toString())
         }
      }
}

static void save_value_session()
{
   U_TRACE(5, "::save_value_session()")

   if (UServer_Base::preforked_num_kids == 0) tbl_session->insert(*cookie, session);
   else
      {
      UString data = session->toString();

      if (data.empty() == false)
         {
         int result = rdb_session->store(*cookie, data, RDB_REPLACE);

         if (result) U_SRV_LOG_VAR("error '%d' on store session data...", result);
         }

      delete session;
      }

   session = 0;
}

static void execute_query(UClientImage_Base* client_image)
{
   U_TRACE(5, "::execute_query(%p)", client_image)

   // set session cookie

   if (cookie->empty())
      {
      static uint32_t counter;

      UString ip_client = UClientImage_Base::getRemoteIP();

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

static void init()
{
   U_TRACE(5, "::init()")

   U_INTERNAL_ASSERT_EQUALS(ir, 0)

   ir = U_NEW(IR);

   UString cfg_index;

   bool ok = (ir->loadFileConfig(cfg_index) && ir->openCDB(false));

   if (ok == false) U_ERROR("error on init query application...", 0);

   query            = U_NEW(Query);
   cache            = U_NEW(UCache);
   crono            = U_NEW(UCrono);
   cookie           = U_NEW(UString);
   output           = U_NEW(UString);
   set_cookie       = U_NEW(UString(80U));
   link_paginazione = U_NEW(UString);

   U_INTERNAL_DUMP("UServer_Base::preforked_num_kids = %u", UServer_Base::preforked_num_kids)

   if (UServer_Base::preforked_num_kids == 0)
      {
      tbl_session = U_NEW(UHashMap<DataSession*>);

      tbl_session->allocate(SESSION_SIZE);
      }
   else
      {
      rdb_session = U_NEW(URDB(U_STRING_FROM_CONSTANT(DB_SESSION), false));

      // NB: the old sessions are automatically invalid...
      // (UServer generate the crypto key at startup...) 

      if (rdb_session->open(SESSION_SIZE, O_TRUNC) == false) U_ERROR("error on open session db %S...", DB_SESSION);
      }

   footer = U_NEW(UString(200U));

   footer->snprintf("%s, with %u documents and %u words.", VERSION, cdb_names->size(), cdb_words->size());

   bool exist = cache->open(U_STRING_FROM_CONSTANT("IR/WEB/form/cache.frm"), 32 * 1024);

   if (exist == false) cache->loadContentOf(U_STRING_FROM_CONSTANT(FORM_FILE_DIR_DEFAULT));
}

// (Connection-wide hooks)...

static void reset()
{
   U_TRACE(5, "::reset()")

   U_INTERNAL_ASSERT_POINTER(cookie)
   U_INTERNAL_ASSERT_POINTER(output)
   U_INTERNAL_ASSERT_POINTER(link_paginazione)

             cookie->clear();
             output->clear();
   link_paginazione->clear();
}

static void end()
{
   U_TRACE(5, "::end()")

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

U_EXPORT void runDynamicPage(UClientImage_Base* client_image)
{
   U_TRACE(5, "::runDynamicPage(%p)", client_image)

        if (client_image ==         0) {  init(); return; } //  init (    Server-wide hooks)...
   else if (client_image == (void*)-1) { reset(); return; } // reset (Connection-wide hooks)...
   else if (client_image == (void*)-2) {   end(); return; } //   end

   U_INTERNAL_DUMP("method = %.*S uri = %.*S", U_HTTP_METHOD_TO_TRACE, U_HTTP_URI_TO_TRACE)

   U_INTERNAL_ASSERT_POINTER(UHTTP::form_name_value)
   U_INTERNAL_ASSERT_POINTER(UClientImage_Base::_value)
   U_INTERNAL_ASSERT_POINTER(UClientImage_Base::_buffer)
   U_INTERNAL_ASSERT_POINTER(UClientImage_Base::rbuffer)
   U_INTERNAL_ASSERT_POINTER(UClientImage_Base::wbuffer)
   U_INTERNAL_ASSERT_POINTER(UClientImage_Base::_encoded)
   U_INTERNAL_ASSERT_EQUALS( UClientImage_Base::pClientImage, client_image)

   set_ENV(*client_image->rbuffer);

   uint32_t n = UHTTP::form_name_value->size() / 2;

   if (UHTTP::isHttpGET())
      {
      if (n == 0)
         {
         view_form_without_help();
         }
      else if (n == 1)
         {
         if ((*UHTTP::form_name_value)[1] == U_STRING_FROM_CONSTANT("help"))
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

      session = U_NEW(DataSession(QUERY, (*UHTTP::form_name_value)[3].strtol()));

      execute_query(client_image);

      save_value_session();
      }

   if (output->empty() == false)
      {
      if (set_cookie->empty() == false)
         {
         (void) client_image->wbuffer->append(*set_cookie);

         set_cookie->setEmpty();
         }

      (void) client_image->wbuffer->append(U_CONSTANT_TO_PARAM("Content-Type: " U_CTYPE_HTML "\r\n\r\n"));
      (void) client_image->wbuffer->append(*output);

      return;
      }

bad_request:
   UHTTP::setHTTPBadRequest();
}
}
