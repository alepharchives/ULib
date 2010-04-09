// verify.cpp

#include <ulib/file_config.h>
#include <ulib/xml/libxml2/schema.h>

#include "context.h"

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

#define U_SCHEMA (const char*)(argv[optind+1])

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
         cfg_str = opt['c'];
         }

      // manage file configuration

      if (cfg_str.empty()) cfg_str = U_STRING_FROM_CONSTANT("XAdES.ini");

      // ----------------------------------------------------------------------------------------------------------------------------------
      // XAdES - configuration parameters
      // ----------------------------------------------------------------------------------------------------------------------------------
      // DigestAlgorithm   md2 | md5 | sha | sha1 | sha224 | sha256 | sha384 | sha512 | mdc2 | ripmed160
      //
      // SigningTime this property contains the time at which the signer claims to have performed the signing process (yes/no)
      // ClaimedRole this property contains claimed or certified roles assumed by the signer in creating the signature
      //
      // this property contains the indication of the purported place where the signer claims to have produced the signature
      // -------------------------------------------------------------------------------------------------------------------
      // ProductionPlaceCity
      // ProductionPlaceStateOrProvince
      // ProductionPlacePostalCode
      // ProductionPlaceCountryName
      // -------------------------------------------------------------------------------------------------------------------
      //
      // DataObjectFormatMimeType   this property identifies the format of a signed data object (when electronic signatures
      //                            are not exchanged in a restricted context) to enable the presentation to the verifier or
      //                            use by the verifier (text, sound or video) in exactly the same way as intended by the signer
      //
      // CAStore
      // ArchiveTimeStamp           the time-stamp token within this property covers the archive validation data
      //
      // SignatureTimeStamp         the time-stamp token within this property covers the digital signature value element
      // Schema                     the pathname XML Schema of XAdES
      // ----------------------------------------------------------------------------------------------------------------------------------

      (void) cfg.open(cfg_str);

      schema = ( U_SCHEMA == 0 ||
                *U_SCHEMA == '\0'
                  ? cfg[U_STRING_FROM_CONSTANT("XAdES-L.Schema")]
                  : UString(U_SCHEMA));

      if (schema.empty()) U_ERROR("error on XAdES schema: empty", 0);

      UXML2Schema XAdES_schema(UFile::contentOf(schema));

      // manage arg operation

      UFile doc;
      const char* file;
      UDSIGContext dsigCtx;
      UString content, pathfile;

      UApplication::exit_value = 1;

      for (uint32_t i = 1; (file = argv[optind]); ++i, ++optind)
         {
         (void) pathfile.assign(file);

         doc.setPath(pathfile);

         content = doc.getContent();

         if (U_MEMCMP(doc.getMimeType(), "application/xml") != 0)
            {
            U_WARNING("I can't verify this kind of document: %.*S", U_FILE_TO_TRACE(doc));

            continue;
            }

         UXML2Document document(content);

         if (XAdES_schema.validate(document) == false ||
             dsigCtx.verify( document) == false)
            {
            continue;
            }

         UApplication::exit_value = 0;

         break;
         }
      }

private:
   int alg;
   UFileConfig cfg;
   UString cfg_str, schema;
};

U_MAIN(Application)
