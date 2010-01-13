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

   bool processReference(xmlNodePtr ref)
      {
      U_TRACE(5, "Application::processReference(%p)", ref)

      /* Reference is an element that may occur one or more times. It specifies
       * a digest algorithm and digest value, and optionally an identifier of the 
       * object being signed, the type of the object, and/or a list of transforms 
       * to be applied prior to digesting. The identification (URI) and transforms 
       * describe how the digested content (i.e., the input to the digest method) 
       * was created. The Type attribute facilitates the processing of referenced 
       * data. For example, while this specification makes no requirements over 
       * external data, an application may wish to signal that the referent is a 
       * Manifest. An optional ID attribute permits a Reference to be referenced 
       * from elsewhere
       */

      const char* id           = 0;
      const char* uri          = 0;
      const char* type         = 0;
      const char* digestMethod = 0;

      xmlNodePtr node, digestValueNode = 0;

      while (ref &&
             UXML2Node(ref).checkNodeName((const xmlChar*)"Reference",
                                          (const xmlChar*)"http://www.w3.org/2000/09/xmldsig#"))
         {
         // read attributes first

         id   = UXML2Node::getProp(ref, "Id");
         uri  = UXML2Node::getProp(ref, "URI");
         type = UXML2Node::getProp(ref, "Type");

         // first is optional Transforms node

         node = UXML2Node::getNextSibling(ref->children);

         if (node &&
             UXML2Node(node).checkNodeName((const xmlChar*)"Transforms",
                                           (const xmlChar*)"http://www.w3.org/2000/09/xmldsig#"))
            {
            node = UXML2Node::getNextSibling(node->next);
            }

         // next node is required DigestMethod

         if (node &&
             UXML2Node(node).checkNodeName((const xmlChar*)"DigestMethod",
                                           (const xmlChar*)"http://www.w3.org/2000/09/xmldsig#"))
            {
            digestMethod = UXML2Node::getProp(node, "Algorithm");

            node = UXML2Node::getNextSibling(node->next);
            }

         // last node is required DigestValue

         if (node &&
             UXML2Node(node).checkNodeName((const xmlChar*)"DigestValue",
                                           (const xmlChar*)"http://www.w3.org/2000/09/xmldsig#"))
            {
            digestValueNode = node;
            }

         // if there is something left than it's an error

         if (node) U_RETURN(false);

         ref = UXML2Node::getNextSibling(ref->next);
         }

      U_RETURN(true);
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

         if (U_MEMCMP(doc.getMimeType(), "application/xml") == 0)
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

            const char* signature_id = UXML2Node::getProp(signature, "Id"); 

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

            if (processReference(UXML2Node::getNextSibling(cur->next)) == false) continue;

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
