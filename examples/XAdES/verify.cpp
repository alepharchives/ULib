// verify.cpp

#include <ulib/url.h>
#include <ulib/date.h>
#include <ulib/file_config.h>
#include <ulib/base/ssl/dgst.h>
#include <ulib/utility/base64.h>
#include <ulib/ssl/certificate.h>
#include <ulib/utility/services.h>
#include <ulib/utility/xml_escape.h>
#include <ulib/utility/string_ext.h>
#include <ulib/xml/libxml2/schema.h>

#ifdef HAVE_SSL_TS
#  include <ulib/ssl/timestamp.h>
#endif

#undef  PACKAGE
#define PACKAGE "verify"

#define ARGS "[file1 file2 file3...]"

#define U_OPTIONS \
"purpose \"simple XAdES verify...\"\n" \
"option c config 1 \"path of configuration file\" \"\"\n"

#include <ulib/application.h>

class Application : public UApplication {
public:

   ~Application()
      {
      U_TRACE(5, "Application::~Application()")
      }

   void run(int argc, char* argv[], char* env[])
      {
      U_TRACE(5, "Application::run(%d,%p,%p)", argc, argv, env)

      UApplication::run(argc, argv, env);

      // manage options

      if (UApplication::isOptions())
         {
         cfg_str = (*opt)['c'];
         }

      // manage file configuration

      if (cfg_str.empty()) cfg_str = U_STRING_FROM_CONSTANT("verify.cfg");

      // ----------------------------------------------------------------------------------------------------------------------------------
      // XAdES verify - configuration parameters
      // ----------------------------------------------------------------------------------------------------------------------------------
      // SCHEMA
      // ----------------------------------------------------------------------------------------------------------------------------------

      (void) cfg.open(cfg_str);

      UXML2Schema schema(UFile::contentOf(cfg[U_STRING_FROM_CONSTANT("SCHEMA")]));

      // manage arg operation

      UFile doc;
      const char* file;
      UString content, pathfile;

      UApplication::exit_value = 1;

      for (uint32_t i = 1; (file = argv[optind]); ++i, ++optind)
         {
         (void) pathfile.assign(file);

         doc.setPath(pathfile);

         content = doc.getContent();

         if (U_MEMCMP(doc.getMimeType(), "application/xml"))
            {
            U_WARNING("I can't verify this kind of document: %.*S", U_FILE_TO_TRACE(doc));
            }
         else
            {
            UXML2Document document(content);

            if (schema.validate(document))
               {
               // find start node

               xmlNodePtr node = document.findNode(document.getRootNode(),
                                                      (const xmlChar*)"Signature",
                                                      (const xmlChar*)"http://www.w3.org/2000/09/xmldsig#");

               if (node)
                  {
                  UApplication::exit_value = 0;
                  }
               }
            }
         }
      }

private:
   int alg;
   UFileConfig cfg;
   UString cfg_str;
};

U_MAIN(Application)
