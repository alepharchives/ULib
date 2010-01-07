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

         if (U_MEMCMP(doc.getMimeType(), "application/xml") == false)
            {
            U_WARNING("I can't verify this kind of document: %.*S", U_FILE_TO_TRACE(doc));

            continue;
            }

         UXML2Document document(content);

         if (schema.validate(document))
            {
            // find signature node (the Signature element is the root element of an XML Signature)

            xmlNodePtr signature = document.findNode(document.getRootNode(),
                                                   (const xmlChar*)"Signature",
                                                   (const xmlChar*)"http://www.w3.org/2000/09/xmldsig#");

            if (signature == 0) continue;

            const char* id = UXML2Node::getProp(signature, "Id"); 

            // first node is required SignedInfo

            xmlNodePtr signedInfoNode = UXML2Node::getNextSibling(signature->children);

            if (signedInfoNode == 0) continue;

            // next node is required SignatureValue

            xmlNodePtr signValueNode = UXML2Node::getNextSibling(signedInfoNode->next);

            if (signValueNode == 0 ||
                UXML2Node(signValueNode).checkNodeName((const xmlChar*)"SignatureValue",
                                                       (const xmlChar*)"http://www.w3.org/2000/09/xmldsig#") == false) continue;

            // next node is optional KeyInfo

            xmlNodePtr keyInfoNode;
            xmlNodePtr cur = UXML2Node::getNextSibling(signValueNode->next);

            if (cur &&
                UXML2Node(cur).checkNodeName((const xmlChar*)"KeyInfo",
                                             (const xmlChar*)"http://www.w3.org/2000/09/xmldsig#") == false)
               {
               keyInfoNode = 0;
               }
            else
               {
               keyInfoNode = cur;
               cur         = UXML2Node::getNextSibling(cur->next);
               }

            // next nodes are optional Object nodes

            while (cur &&
                   UXML2Node(cur).checkNodeName((const xmlChar*)"Object",
                                                (const xmlChar*)"http://www.w3.org/2000/09/xmldsig#"))
               {
               /* Object is an optional element that may occur one or more times. When present,
                * this element may contain any data. The Object element may include optional MIME
                * type, ID, and encoding attributes.
                */

               cur = UXML2Node::getNextSibling(cur->next);
               }

            // if there is something left than it's an error

            if (cur) continue;

            // now validated all the references and prepare transform

            /* The SignedInfo Element (http://www.w3.org/TR/xmldsig-core/#sec-SignedInfo)
             * 
             * The structure of SignedInfo includes the canonicalization algorithm, 
             * a result algorithm, and one or more references. The SignedInfo element 
             * may contain an optional ID attribute that will allow it to be referenced by
             * other signatures and objects.i
             *
             * SignedInfo does not include explicit result or digest properties (such as
             * calculation time, cryptographic device serial number, etc.). If an application
             * needs to associate properties with the result or digest, it may include such
             * information in a SignatureProperties element within an Object element.
             */

             // first node is required CanonicalizationMethod

            cur = UXML2Node::getNextSibling(signedInfoNode->children);

            if (cur == 0 ||
                UXML2Node(cur).checkNodeName((const xmlChar*)"CanonicalizationMethod",
                                             (const xmlChar*)"http://www.w3.org/2000/09/xmldsig#") == false) continue;


            const char* c14nMethod = UXML2Node::getProp(cur, "Algorithm");

            if (U_STREQ(c14nMethod, "http://www.w3.org/TR/2001/REC-xml-c14n-20010315") == false) continue;

            // next node is required SignatureMethod

            cur = UXML2Node::getNextSibling(cur->next);

            if (cur == 0 ||
                UXML2Node(cur).checkNodeName((const xmlChar*)"SignatureMethod",
                                             (const xmlChar*)"http://www.w3.org/2000/09/xmldsig#") == false) continue;

            const char* signMethod = UXML2Node::getProp(cur, "Algorithm");

            // calculate references

            cur = UXML2Node::getNextSibling(cur->next);

            while (cur &&
                   UXML2Node(cur).checkNodeName((const xmlChar*)"Reference",
                                                (const xmlChar*)"http://www.w3.org/2000/09/xmldsig#"))
               {
               // create reference

               cur = UXML2Node::getNextSibling(cur->next);
               }

            UApplication::exit_value = 0;
            }
         }
      }

private:
   int alg;
   UFileConfig cfg;
   UString cfg_str;
};

U_MAIN(Application)
