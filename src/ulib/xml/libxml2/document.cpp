// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    document.cpp - wrapping of libxml2
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/xml/libxml2/document.h>

#include <libxml/c14n.h>

bool UXML2Document::binit;

void UXML2Document::init()
{
   U_TRACE(1, "UXML2Document::init()")

   binit = true;

   /*
    * build an XML tree from a file; we need to add default
    * attributes and resolve all character and entities references
    */

   xmlLoadExtDtdDefaultValue = XML_DETECT_IDS | XML_COMPLETE_ATTRS;

   U_SYSCALL_VOID(xmlSubstituteEntitiesDefault, "%d", 1);

   /*
    * Do not fetch DTD over network
    */

// xmlExternalEntityLoader defaultEntityLoader = xmlNoNetExternalEntityLoader;

   U_SYSCALL_VOID(xmlSetExternalEntityLoader, "%p", xmlNoNetExternalEntityLoader);

   xmlLoadExtDtdDefaultValue = 0;
}

UXML2Document::UXML2Document(const UString& data)
{
   U_TRACE_REGISTER_OBJECT(0, UXML2Document, "%.*S", U_STRING_TO_TRACE(data))

   if (binit == false) init();

   impl_ = (xmlDocPtr) U_SYSCALL(xmlParseMemory, "%p,%d", U_STRING_TO_PARAM(data));

   if (impl_ == NULL) U_ERROR("unable to parse xml document", 0);

   /*
    * Check the document is of the right kind
    */

   if (getRootNode() == NULL) U_ERROR("empty xml document", 0);
}

xmlNodePtr UXML2Document::findNode(const xmlNodePtr parent, const xmlChar* name, const xmlChar* ns)
{
   U_TRACE(0, "UXML2Document::findNode(%p,%S,%S)", parent, name, ns)

   U_INTERNAL_ASSERT_POINTER(name)

   xmlNodePtr ret;
   xmlNodePtr cur = parent;

   while (cur)
      {
      if (cur->type == XML_ELEMENT_NODE &&
          UXML2Node(cur).checkNodeName(name, ns))
         {
         U_RETURN_POINTER(cur, xmlNode);
         }

      if (cur->children)
         {
         ret = findNode(cur->children, name, ns);

         if (ret) U_RETURN_POINTER(ret, xmlNode);
         }

      cur = cur->next;
      }

   U_RETURN_POINTER(0, xmlNode);
}

xmlNodePtr UXML2Document::findChild(const xmlNodePtr parent, const xmlChar* name, const xmlChar* ns)
{
   U_TRACE(0, "UXML2Document::findChild(%p,%S,%S)", parent, name, ns)

   U_INTERNAL_ASSERT_POINTER(name)
   U_INTERNAL_ASSERT_POINTER(parent)

   xmlNodePtr cur = parent->children;

   while (cur)
      {
      if (cur->type == XML_ELEMENT_NODE &&
          UXML2Node(cur).checkNodeName(name, ns))
         {
         U_RETURN_POINTER(cur, xmlNode);
         }

      cur = cur->next;
      }

   U_RETURN_POINTER(0, xmlNode);
}

xmlNodePtr UXML2Document::findParent(const xmlNodePtr cur, const xmlChar* name, const xmlChar* ns)
{
   U_TRACE(0, "UXML2Document::findParent(%p,%S,%S)", cur, name, ns)

   U_INTERNAL_ASSERT_POINTER(cur)
   U_INTERNAL_ASSERT_POINTER(name)

   xmlNodePtr ret;

   if (cur->type == XML_ELEMENT_NODE &&
       UXML2Node(cur).checkNodeName(name, ns))
      {
      U_RETURN_POINTER(cur, xmlNode);
      }

   if (cur->parent)
      {
      ret = findParent(cur->parent, name, ns);

      if (ret) U_RETURN_POINTER(ret, xmlNode);
      }

   U_RETURN_POINTER(0, xmlNode);
}

bool UXML2Document::writeToFile(const char* filename, const char* encoding, bool formatted)
{
   U_TRACE(1, "UXML2Document::writeToFile(%S,%S,%b)", filename, encoding, formatted)

   U_INTERNAL_ASSERT_POINTER(impl_)

   int oldIndentTreeOutput = 0, oldKeepBlanksDefault = 0;

   if (formatted)
      {
      oldIndentTreeOutput  = xmlIndentTreeOutput;
      xmlIndentTreeOutput  = 1;
      oldKeepBlanksDefault = U_SYSCALL(xmlKeepBlanksDefault, "%d", 1);
      }

   bool result = (U_SYSCALL(xmlSaveFormatFileEnc, "%S,%p,%S,%d", filename, impl_, encoding, formatted) != -1);

   if (formatted)
      {
      xmlIndentTreeOutput = oldIndentTreeOutput;

      (void) U_SYSCALL(xmlKeepBlanksDefault, "%d", oldKeepBlanksDefault);
      }

   U_RETURN(result);
}

xmlChar* UXML2Document::writeToString(int& length, const char* encoding, bool formatted)
{
   U_TRACE(1, "UXML2Document::writeToString(%S,%b)", encoding, formatted)

   U_INTERNAL_ASSERT_POINTER(impl_)

   int oldIndentTreeOutput, oldKeepBlanksDefault;

   if (formatted)
      {
      oldIndentTreeOutput  = xmlIndentTreeOutput;
      xmlIndentTreeOutput  = 1;
      oldKeepBlanksDefault = U_SYSCALL(xmlKeepBlanksDefault, "%d", 1);
      }

   xmlChar* buffer = 0;

   U_SYSCALL_VOID(xmlDocDumpFormatMemoryEnc, "%p,%p,%p,%S,%d", impl_, &buffer, &length, encoding, formatted);

   if (formatted)
      {
      xmlIndentTreeOutput = oldIndentTreeOutput;

      (void) U_SYSCALL(xmlKeepBlanksDefault, "%d", oldKeepBlanksDefault);
      }

   U_RETURN_POINTER(buffer, xmlChar);
}

// Canonical XML implementation (http://www.w3.org/TR/2001/REC-xml-c14n-20010315)

UString UXML2Document::xmlC14N(int mode, int with_comments, unsigned char** inclusive_namespaces)
{
   U_TRACE(1, "UXML2Document::xmlC14N(%d,%d,%d,%p)", mode, with_comments, inclusive_namespaces)

   U_INTERNAL_ASSERT_POINTER(impl_)

#ifndef LIBXML_C14N_ENABLED
   U_ERROR("XPath/Canonicalization support not compiled in libxml2", 0);
#endif

   /*
    * Canonical form
    */

   UString output;

   xmlChar* result = NULL;

   int ret = U_SYSCALL(xmlC14NDocDumpMemory, "%p,%p,%d,%p,%d,%p", impl_, NULL, mode, inclusive_namespaces, with_comments, &result);

   if (ret < 0) U_WARNING("failed to canonicalize buffer data (%d)", ret);

   if (result != NULL)
      {
      (void) output.replace((const char*)result);

      U_SYSCALL_VOID(xmlFree, "%p", result);
      }

   U_RETURN_STRING(output);
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UXML2Document::dump(bool reset) const
{
   *UObjectIO::os << "impl_ " << (void*)impl_;

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
