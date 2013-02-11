// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    attribute.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/xml/expat/attribute.h>

// STREAM

U_EXPORT ostream& operator<<(ostream& os, const UXMLAttribute& a)
{
   U_TRACE(0+256, "UXMLAttribute::operator<<(%p,%p)", &os, &a)

   os.put('{');
   os.put(' ');

   if (a.namespaceName.empty() == false)
      {
      os << a.namespaceName;

      os.put(':');
      }

   os << a.accessor;
   os.put('=');
   os << a.value;
   os.put(' ');
   os.put('}');

   return os;
}

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UXMLAttribute::dump(bool reset) const
{
   *UObjectIO::os << "str           (UString " << (void*)&str           << ")\n"
                  << "value         (UString " << (void*)&value         << ")\n"
                  << "accessor      (UString " << (void*)&accessor      << ")\n"
                  << "namespaceName (UString " << (void*)&namespaceName << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}
#endif
