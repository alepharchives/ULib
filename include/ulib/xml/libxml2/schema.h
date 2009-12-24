// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    schema.h - wrapping of libxml2
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_UXML2SCHEMA_H
#define ULIB_UXML2SCHEMA_H 1

#include <ulib/xml/libxml2/node.h>

#include <libxml/xmlschemas.h>
#include <libxml/xmlschemastypes.h>

/*
A Schemas definition

struct _xmlSchema {
   const xmlChar*    name;             // schema name
   const xmlChar*    targetNamespace;  // the target namespace
   const xmlChar*    version;
   const xmlChar*    id;               // Obsolete
   xmlDocPtr         doc;
   xmlSchemaAnnotPtr annot;
   int               flags;

   xmlHashTablePtr   typeDecl;
   xmlHashTablePtr   attrDecl;
   xmlHashTablePtr   attrgrpDecl;
   xmlHashTablePtr   elemDecl;
   xmlHashTablePtr   notaDecl;
   xmlHashTablePtr   schemasImports;

   void*             _private;         // unused by the library for users or bindings
   xmlHashTablePtr   groupDecl;
   xmlDictPtr        dict;
   void*             includes;         // the includes, this is opaque for now
   int               preserve;         // whether to free the document
   int               counter;          // used to give ononymous components unique names
   xmlHashTablePtr   idcDef;           // All identity-constraint defs
   void*             volatiles;        // Obsolete
};
*/

class U_EXPORT UXML2Schema {
public:

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   /** Create a schema from a XML document.
   *
   * @param XMLSchema document
   */

   UXML2Schema(const UString& xmldoc)
      {
      U_TRACE_REGISTER_OBJECT(0, UXML2Schema, "%.*S", U_STRING_TO_TRACE(xmldoc))

      xmlSchemaParserCtxtPtr context = (xmlSchemaParserCtxtPtr) U_SYSCALL(xmlSchemaNewMemParserCtxt, "%S,%u", U_STRING_TO_PARAM(xmldoc));

      ctxt  = 0;
      impl_ = (xmlSchemaPtr) U_SYSCALL(xmlSchemaParse, "%p", context);

      impl_->_private = this;

      U_SYSCALL_VOID(xmlSchemaFreeParserCtxt, "%p", context);
      }

   ~UXML2Schema()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UXML2Schema)

                U_SYSCALL_VOID(xmlSchemaFree,          "%p", impl_);
      if (ctxt) U_SYSCALL_VOID(xmlSchemaFreeValidCtxt, "%p", ctxt);
      }

   const char* getName() const
      {
      U_TRACE(0, "UXML2Schema::getName()")

      U_INTERNAL_ASSERT_POINTER(impl_)

      const char* result = (impl_->name ? (const char*)impl_->name : "");

      U_RETURN(result);
      }

   const char* getTargetNameSpace() const
      {
      U_TRACE(0, "UXML2Schema::getTargetNameSpace()")

      U_INTERNAL_ASSERT_POINTER(impl_)

      const char* result = (impl_->targetNamespace ? (const char*)impl_->targetNamespace : "");

      U_RETURN(result);
      }

   const char* getVersion() const
      {
      U_TRACE(0, "UXML2Schema::getVersion()")

      U_INTERNAL_ASSERT_POINTER(impl_)

      const char* result = (impl_->version ? (const char*)impl_->version : "");

      U_RETURN(result);
      }

   bool validate(xmlDocPtr doc);
   bool validate(const UString& docfile);

   // Access the underlying libxml2 implementation.

   xmlSchemaPtr cobj() { return impl_; }

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   xmlSchemaPtr impl_;
   xmlSchemaValidCtxtPtr ctxt;

   /** Create a schema from the underlying libxml schema element.
   */

   UXML2Schema(xmlSchemaPtr schema) : impl_(schema)
      {
      U_TRACE_REGISTER_OBJECT(0, UXML2Schema, "%p", schema)

      impl_->_private = this;
      }

private:
   UXML2Schema(const UXML2Schema&)            {}
   UXML2Schema& operator=(const UXML2Schema&) { return *this; }
};

#endif
