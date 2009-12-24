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

#ifdef HAVE_SSL_TS
#  include <ulib/ssl/timestamp.h>
#endif

#undef  PACKAGE
#define PACKAGE "verify"

#define ARGS "[file]"

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

   /*
   bool xmlIsValidAgainstXSDSchema(xmlDocPtr doc, const UString& xsd)
      {
      U_TRACE(5, "Application::xmlIsValidAgainstXSDSchema(%p,%.*S)", doc, U_STRING_TO_TRACE(xsd))

      int i;
      xmlSchemaValidCtxtPtr ctxt2 = NULL;

      xmlSchemaParserCtxtPtr ctxt = xmlSchemaNewParserCtxt(ops.schema);

      xmlSchemaSetParserErrors(ctxt, (xmlSchemaValidityErrorFunc)NULL, (xmlSchemaValidityWarningFunc) NULL, NULL);

      xmlGenericError        = foo;
      xmlGenericErrorContext = NULL;

      xmlSchemaPtr schema = xmlSchemaParse(ctxt);

      xmlSchemaFreeParserCtxt(ctxt);

      if (schema != NULL)
         {                  
         for (i=start; i<argc; i++)
            {
            xmlDocPtr doc;
            int ret;

            ret = 0;
            doc = NULL;

            ctxt2 = xmlSchemaNewValidCtxt(schema);

            if (!ops.err)
            {
            xmlSchemaSetValidErrors(ctxt2,
               (xmlSchemaValidityErrorFunc) NULL,
               (xmlSchemaValidityWarningFunc) NULL,
               NULL);
            xmlGenericError = foo;
            xmlGenericErrorContext = NULL;
            xmlInitParser();
            }
            else
            {
            xmlSchemaSetValidErrors(ctxt2,
               (xmlSchemaValidityErrorFunc) fprintf,
               (xmlSchemaValidityWarningFunc) fprintf,
               stderr);
            }

            if (!ops.err)
            {
            xmlDefaultSAXHandlerInit();
            xmlDefaultSAXHandler.error = NULL;
            xmlDefaultSAXHandler.warning = NULL;
            }

            // doc = xmlParseFile(argv[i]);
            doc = xmlReadFile(argv[i], NULL, 0);
            if (doc)
            {
            ret = xmlSchemaValidateDoc(ctxt2, doc);
            xmlFreeDoc(doc);
            }
            else
            {
            ret = 1; // Malformed XML or could not open file
            }
            if (ret) invalidFound = 1;

            if (!ops.show_val_res)
            {
            if ((ops.listGood > 0) && (ret == 0))
               fprintf(stdout, "%s\n", argv[i]);
            if ((ops.listGood < 0) && (ret != 0))
               fprintf(stdout, "%s\n", argv[i]);
            }
            else
            {
            if (ret == 0)
               fprintf(stdout, "%s - valid\n", argv[i]);
            else
               fprintf(stdout, "%s - invalid\n", argv[i]);
            }

            if (ctxt2 != NULL) xmlSchemaFreeValidCtxt(ctxt2);
            }
         }
      else
      {
      invalidFound = 2;
      }

      if (schema != NULL) xmlSchemaFree(schema);

      xmlSchemaCleanupTypes();        
      }
   */

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
      // ----------------------------------------------------------------------------------------------------------------------------------

      (void) cfg.open(cfg_str);

      // manage arg operation

      const char* file;

      document = UFile::contentOf(file);

      if (document.empty()    ||
          document.isBinary() ||
          u_endsWith(file, strlen(file), U_CONSTANT_TO_PARAM(".xml")) == false) U_ERROR("I can't verify this document: %s", file);
      }

private:
   int alg;
   UFileConfig cfg;
   UString cfg_str, document;
};

U_MAIN(Application)
