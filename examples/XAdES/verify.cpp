// verify.cpp

#include <ulib/file_config.h>
#include <ulib/utility/base64.h>
#include <ulib/utility/services.h>
#include <ulib/xml/libxml2/schema.h>

#ifdef HAVE_SSL_TS
#  include <ulib/ssl/timestamp.h>
#endif

#include "utility.h"
#include "context.h"

#undef  PACKAGE
#define PACKAGE "verify"

#define ARGS "[ SCHEMA ]"

#define U_OPTIONS \
"purpose \"XAdES verify\"\n" \
"option c config 1 \"path of configuration file\" \"\"\n"

#include <ulib/application.h>

#define U_SCHEMA (const char*)(argv[optind+0])

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

      if (UApplication::isOptions()) cfg_str = opt['c'];

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

      UString x(U_CAPACITY);

      UServices::readEOF(STDIN_FILENO, x);

      if (x.empty()) U_ERROR("cannot read data from <stdin>...");

      (void) content.reserve(x.size());

      if (UBase64::decode(x, content) == false) U_ERROR("decoding data read failed...");

      // manage arguments...

      schema = ( U_SCHEMA == 0 ||
                *U_SCHEMA == '\0'
                  ? cfg[U_STRING_FROM_CONSTANT("XAdES-L.Schema")]
                  : UString(U_SCHEMA));

      if (schema.empty()) U_ERROR("error on XAdES schema: empty");

      UXML2Schema XAdES_schema(UFile::contentOf(schema));

      // ---------------------------------------------------------------------------------------------------------------
      // check for OOffice or MS-Word document...
      // ---------------------------------------------------------------------------------------------------------------
      utility.handlerConfig(cfg);

      if (utility.checkDocument(content, "XAdES", false)) content = utility.getSigned();
      // ---------------------------------------------------------------------------------------------------------------

      UApplication::exit_value = 1;

      UXML2Document document(content);

      if (XAdES_schema.validate(document) == false) U_ERROR("error on input data: not XAdES");

      UDSIGContext dsigCtx;

      if (dsigCtx.verify(document)) UApplication::exit_value = 0;

      utility.clean();
      }

private:
   int alg;
   UFileConfig cfg;
   UXAdESUtility utility;
   UString cfg_str, content, schema;
};

U_MAIN(Application)
