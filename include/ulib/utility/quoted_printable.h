// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    quoted_printable.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_QUOTED_PRINTABLE_H
#define ULIB_QUOTED_PRINTABLE_H 1

#include <ulib/base/coder/quoted_printable.h>

#include <ulib/string.h>

struct U_EXPORT UQuotedPrintable {

   static void encode(const unsigned char* s, uint32_t n, UString& buffer)
      {
      U_TRACE(0, "UQuotedPrintable::encode(%.*S,%u,%.*S)", n, s, n, U_STRING_TO_TRACE(buffer))

#  ifdef DEBUG
      uint32_t length = ((n * 3) + 4);

      U_INTERNAL_DUMP("buffer.capacity() = %u length = %u", buffer.capacity(), length)

      U_ASSERT(buffer.capacity() >= length + 1)
#  endif

      uint32_t pos = u_quoted_printable_encode(s, n, (unsigned char*)buffer.data());

      buffer.size_adjust(pos);
      }

   static void encode(const UString& s, UString& buffer)
      { encode((const unsigned char*)U_STRING_TO_PARAM(s), buffer); }

   static bool decode(const char* s, uint32_t n, UString& buffer)
      {
      U_TRACE(0, "UQuotedPrintable::decode(%.*S,%u,%.*S)", n, s, n, U_STRING_TO_TRACE(buffer))

      uint32_t pos = u_quoted_printable_decode(s, n, (unsigned char*)buffer.data());

      buffer.size_adjust(pos);

      bool result = (pos > 0);

      U_RETURN(result);
      }

   static bool decode(const UString& s, UString& buffer)
      { return decode(U_STRING_TO_PARAM(s), buffer); }
};

#endif
