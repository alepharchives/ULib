// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    escape.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_ESCAPE_H
#define ULIB_ESCAPE_H 1

#include <ulib/base/coder/escape.h>

#include <ulib/string.h>

/* Encode-Decode escape sequences into a buffer, the following are recognized:
 * ---------------------------------------------------------------------------
 * \0  NUL
 * \b  BS  backspace          (\010  8  8)
 * \t  HT  horizontal tab     (\011  9  9)
 * \n  LF  newline            (\012 10  A)
 * \v  VT  vertical tab       (\013 11  B)
 * \f  FF  formfeed           (\014 12  C)
 * \r  CR  carriage return    (\015 13  D)
 * \e  ESC character          (\033 27 1B)
 *
 * \^C   C = any letter (Control code)
 * \xDD  number formed of 1-2 hex   digits
 * \DDD  number formed of 1-3 octal digits
 * ---------------------------------------------------------------------------
 */

struct U_EXPORT UEscape {

   static void encode(const unsigned char* s, uint32_t n, UString& buffer)
      {
      U_TRACE(0, "UEscape::encode(%.*S,%u,%.*S)", n, s, n, U_STRING_TO_TRACE(buffer))

#  ifdef DEBUG
      uint32_t length = ((n * 3) + 4);

      U_INTERNAL_DUMP("buffer.capacity() = %u length = %u", buffer.capacity(), length)

      U_ASSERT(buffer.capacity() >= length + 1)
#  endif

      uint32_t pos = u_escape_encode(s, n, buffer.data(), buffer.capacity());

      buffer.size_adjust(pos);
      }

   static void encode(const UString& s, UString& buffer)
      { encode((const unsigned char*)U_STRING_TO_PARAM(s), buffer); }

   static bool decode(const char* s, uint32_t n, UString& buffer)
      {
      U_TRACE(0, "UEscape::decode(%.*S,%u,%.*S)", n, s, n, U_STRING_TO_TRACE(buffer))

      uint32_t pos = u_escape_decode(s, n, (unsigned char*)buffer.data());

      buffer.size_adjust(pos);

      bool result = (pos > 0);

      U_RETURN(result);
      }

   static bool decode(const UString& s, UString& buffer)
      { return decode(U_STRING_TO_PARAM(s), buffer); }
};

#endif
