// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    xml_escape.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_XML_ESCAPE_H
#define ULIB_XML_ESCAPE_H 1

#include <ulib/base/coder/xml.h>

#include <ulib/string.h>

struct U_EXPORT UXMLEscape {

   static void encode(const unsigned char* s, uint32_t n, UString& buffer)
      {
      U_TRACE(0, "UXMLEscape::encode(%.*S,%u,%.*S)", n, s, n, U_STRING_TO_TRACE(buffer))

#  ifdef DEBUG
      uint32_t length = ((n * 3) + 4);

      U_INTERNAL_DUMP("buffer.capacity() = %u length = %u", buffer.capacity(), length)

      U_ASSERT(buffer.capacity() >= length + 1)
#  endif

      uint32_t pos = u_xml_encode(s, n, (unsigned char*)buffer.data());

      buffer.size_adjust(pos);
      }

   static void encode(const UString& s, UString& buffer)
      { encode((const unsigned char*)U_STRING_TO_PARAM(s), buffer); }

   static bool decode(const unsigned char* s, uint32_t n, UString& buffer)
      {
      U_TRACE(0, "UXMLEscape::decode(%.*S,%u,%.*S)", n, s, n, U_STRING_TO_TRACE(buffer))

      uint32_t pos = u_xml_decode(s, n, (unsigned char*) buffer.data());

      buffer.size_adjust(pos);

      bool result = (pos > 0);

      U_RETURN(result);
      }

   static bool decode(const UString& s, UString& buffer)
      { return decode((const unsigned char*)U_STRING_TO_PARAM(s), buffer); }
};

#endif
