// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    node.cpp - wrapping of libxml2
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/xml/libxml2/node.h>

uint32_t UXML2Node::getChildren(UVector<xmlNodePtr>& children, const xmlChar* name) const
{
   U_TRACE(0, "UXML2Node::getChildren(%p,%S)", &children, name)

   U_INTERNAL_ASSERT_POINTER(impl_)

   xmlNodePtr child = impl_->children;

   if (child == 0) U_RETURN(0);

   uint32_t n = children.size();

   do {
      if ((name == 0 || strcmp((const char*)name, (const char*)child->name) == 0)) children.push_back(child);
      }
   while ((child = child->next));

   U_RETURN(children.size() - n);
}

xmlNodePtr UXML2Node::createNewChildNode(const xmlChar* name, const xmlChar* ns_prefix)
{
   U_TRACE(1, "UXML2Node::createNewChildNode(%S,%S)", name, ns_prefix)

   U_INTERNAL_ASSERT_POINTER(impl_)
   U_ASSERT_DIFFERS(impl_->type, XML_ELEMENT_NODE)

   xmlNsPtr ns = 0;

   // Ignore the namespace if none was specified

   if (ns_prefix)
      {
      // Use the existing namespace if one exists

      ns = getNamespace(ns_prefix);

      U_INTERNAL_ASSERT_POINTER(ns)
      }

   xmlNodePtr node = (xmlNodePtr) U_SYSCALL(xmlNewNode, "%p,%S", ns, name);

   U_RETURN_POINTER(node,_xmlNode);
}

uint32_t UXML2Node::find_impl(UVector<xmlNodePtr>& vec, xmlXPathContext* ctxt, const xmlChar* xpath)
{
   U_TRACE(1, "UXML2Node::find_impl(%p,%p,%S)", &vec, ctxt, xpath)

   xmlNodeSetPtr nodeset;
   uint32_t n = vec.size();
   xmlXPathObjectPtr result = (xmlXPathObjectPtr) U_SYSCALL(xmlXPathEval, "%S,%p", xpath, ctxt);

   if (result       == 0)             goto ctx;
   if (result->type != XPATH_NODESET) goto path;

   nodeset = result->nodesetval;

   /*
   struct _xmlNodeSet {
      int nodeNr;          // number of nodes in the set
      int nodeMax;         // size of the array as allocated
      xmlNodePtr* nodeTab; // array of nodes in no particular order
   };
   */

   if (nodeset)
      {
      vec.reserve(nodeset->nodeNr);

      for (int i = 0; i < nodeset->nodeNr; ++i) vec.push_back(nodeset->nodeTab[i]);
      }

path:
   U_SYSCALL_VOID(xmlXPathFreeObject,  "%p", result);
ctx:
   U_SYSCALL_VOID(xmlXPathFreeContext, "%p", ctxt);

   U_RETURN(vec.size() - n);
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UXML2Node::dump(bool reset) const
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
