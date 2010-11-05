// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    attribute.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_XML_ATTRIBUTE_H
#define ULIB_XML_ATTRIBUTE_H 1

#include <ulib/string.h>

/**
UXMLAttribute just acts as a repository for the value, name, and namespace of any attributes attached to the element.
It never exists without a UXMLElement.
*/

class U_EXPORT UXMLAttribute {
public:

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   // Costruttori

   UXMLAttribute()
      {
      U_TRACE_REGISTER_OBJECT(0, UXMLAttribute, "")
      }

   UXMLAttribute(const UString& s, const UString& a, const UString& n, const UString& v)
         : str(s), value(v), accessor(a), namespaceName(n)
      {
      U_TRACE_REGISTER_OBJECT(0, UXMLAttribute, "%.*S,%.*S,%.*S,%.*S", U_STRING_TO_TRACE(s), U_STRING_TO_TRACE(a),
                                                                       U_STRING_TO_TRACE(n), U_STRING_TO_TRACE(v))
      }

   ~UXMLAttribute()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UXMLAttribute)
      }

   // SERVICES

   UString& getValue()         { return value; }         // reference to the internal value of the attribute
   UString& getAccessor()      { return accessor; }      // reference to the internal accessor
   UString& getNamespaceName() { return namespaceName; } // reference to the internal namespace name

   // STREAM

   friend U_EXPORT ostream& operator<<(ostream& os, const UXMLAttribute& a);

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
    UString str,
            value,         // Value of the attribute
            accessor,      // Name  of the attributes accessor
            namespaceName; // Name  of the namespace associated with the attribute

private:
   UXMLAttribute(const UXMLAttribute&)            {}
   UXMLAttribute& operator=(const UXMLAttribute&) { return *this; }
};

#endif
