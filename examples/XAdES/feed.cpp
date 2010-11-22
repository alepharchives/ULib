// feed.cpp

#include <ulib/zip/zip.h>
#include <ulib/curl/curl.h>
#include <ulib/ssl/pkcs7.h>
#include <ulib/file_config.h>
#include <ulib/ssl/certificate.h>
#include <ulib/utility/string_ext.h>

#undef  PACKAGE
#define PACKAGE "feed"

#define ARGS "[ URI public certificate list ]"

#define U_OPTIONS \
"purpose \"feed CA storage with public certificate list\"\n" \
"option c config 1 \"path of configuration file\" \"\"\n"

#include <ulib/application.h>

#define U_ZIPPONE (const char*)(argv[optind+0])

// --------------------------------------------------------------
// http://www.cnipa.gov.it/site/_files/LISTACER_20100907.zip.p7m
// --------------------------------------------------------------
// La lista dei certificati, completata con le informazioni previste
// dal DPCM 30 marzo 2009 sopra citato, è strutturata in un archivio
// WINZIP non compresso, come insieme di directory, ciascuna dedicata
// ad un certificatore iscritto, ognuna contenente i certificati forniti
// dalle aziende (in formato binario DER ed ove fornito in formato B64)
// ed un file in formato Rich Text Format che presenta le informazioni
// previste dal più volte citato articolo 41.
// --------------------------------------------------------------

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
      // CAStore                    the location of CA and CRL
      // UriPublicListCerticate     the uri of Public Certificate List
      // ArchiveTimeStamp           the time-stamp token within this property covers the archive validation data
      //
      // SignatureTimeStamp         the time-stamp token within this property covers the digital signature value element
      // Schema                     the pathname XML Schema of XAdES
      // ----------------------------------------------------------------------------------------------------------------------------------

      (void) cfg.open(cfg_str);

      UString ca_store = cfg[U_STRING_FROM_CONSTANT("XAdES-C.CAStore")];

      if (ca_store.empty()) U_ERROR("XAdES - CAStore is empty...");

      if (UFile::chdir(ca_store.c_str(), true) == false) U_ERROR("XAdES - chdir on CAStore %S failed...", ca_store.data());

      // manage arguments...

      uri = ( U_ZIPPONE == 0 ||
                *U_ZIPPONE == '\0'
                  ? cfg[U_STRING_FROM_CONSTANT("XAdES-C.UriPublicListCerticate")]
                  : UString(U_ZIPPONE));

      if (uri.empty()) U_ERROR("XAdES - UriPublicListCerticate is empty...");

      curl.setURL(uri.c_str());

      if (curl.performWait(1024U * 1024U))
         {
         UPKCS7 zippone(curl.getResponse(), "DER");

         if (zippone.isValid() == false) U_ERROR("Error reading S/MIME Public Certificate List, may be the file is not signed...");

         UZIP zip(zippone.getContent());

         if (zip.readContent() == false) U_ERROR("Error reading ZIP Public Certificate List, may be the file is not zipped...");

         bool exist;
         long hash_code;
         UVector<UString> vec(5U);
         uint32_t i, j, k, n = zip.getFilesCount();
         UString namefile, hash(100U), item, list, uri_crl;

         U_INTERNAL_DUMP("ZIP: %d parts", n)

         for (i = 0; i < n; ++i)
            {
            namefile = zip.getFilenameAt(i);

            U_INTERNAL_DUMP("Part %d: Filename=%.*S", i+1, U_STRING_TO_TRACE(namefile))

            // .cer .crt .der

            if (UStringExt::endsWith(namefile, U_CONSTANT_TO_PARAM(".cer")) ||
                UStringExt::endsWith(namefile, U_CONSTANT_TO_PARAM(".crt")) ||
                UStringExt::endsWith(namefile, U_CONSTANT_TO_PARAM(".der")))
               {
               UCertificate cert(zip.getFileContentAt(i));

               if (cert.isValid())
                  {
                  // Link a certificate to its subject name hash value, each hash is of
                  // the form <hash>.<n> where n is an integer. If the hash value already exists
                  // then we need to up the value of n, unless its a duplicate in which
                  // case we skip the link. We check for duplicates by comparing the
                  // certificate fingerprints

                  hash_code = cert.hashCode();

                  hash.snprintf("%08x.0", hash_code);

                  exist = UFile::access(hash.data(), R_OK);

                  list = cert.getRevocationURL();

                  if (list.empty()) uri_crl.clear();
                  else
                     {
                     for (j = 0, k = vec.split(list); j < k; ++j)
                        {
                        item = vec[j];

                        if (U_STRNEQ(item.data(), "URI:"))
                           {
                           (void) uri_crl.replace(item.substr(U_CONSTANT_SIZE("URI:")));

                           U_INTERNAL_DUMP("uri_crl = %.*S", U_STRING_TO_TRACE(uri_crl))
                           }
                        }

#                 ifdef DEBUG
                      vec.clear();
                     item.clear();
#                 endif
                     }

                  U_INTERNAL_DUMP("namefile = %.*S hash = %.*S exist = %b", U_STRING_TO_TRACE(namefile), U_STRING_TO_TRACE(hash), exist)

                  if (exist == false)
                     {
                     (void) UFile::writeTo(hash, cert.getEncoded("PEM"));
                     }
                  }
               }
            }
         }

      (void) UFile::chdir(0, true);
      }

private:
   UCURL curl;
   UFileConfig cfg;
   UString cfg_str, uri;
};

U_MAIN(Application)
