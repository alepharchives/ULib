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
 * \r  CR  carriage return    (\015 13  D)
 * \n  LF  newline            (\012 10  A)
 * \t  HT  horizontal tab     (\011  9  9)
 * \b  BS  backspace          (\010  8  8)
 * \f  FF  formfeed           (\014 12  C)
 * \v  VT  vertical tab       (\013 11  B)
 * \a  BEL                    (\007  7  7)
 * \e  ESC character          (\033 27 1B)
 *
 * \u    four-hex-digits (unicode char)
 * \^C   C = any letter (Control code)
 * \xDD  number formed of 1-2 hex   digits
 * \DDD  number formed of 1-3 octal digits
 * ---------------------------------------------------------------------------
 */

struct U_EXPORT UEscape {

   static void encode(const unsigned char* s, uint32_t n, UString& buffer, bool json)
      {
      U_TRACE(0, "UEscape::encode(%.*S,%u,%.*S,%b)", n, s, n, U_STRING_TO_TRACE(buffer), json)

      U_ASSERT(buffer.capacity() >= n)

      uint32_t sz  = buffer.size(),
               pos = u_escape_encode(s, n, buffer.c_pointer(sz), buffer.space(), json);

      buffer.size_adjust(sz + pos);
      }

   static void encode(const UString& s, UString& buffer, bool json)
      { encode((const unsigned char*)U_STRING_TO_PARAM(s), buffer, json); }

   static bool decode(const char* s, uint32_t n, UString& buffer)
      {
      U_TRACE(0, "UEscape::decode(%.*S,%u,%.*S)", n, s, n, U_STRING_TO_TRACE(buffer))

      U_ASSERT(buffer.capacity() >= n)

      uint32_t pos = u_escape_decode(s, n, (unsigned char*)buffer.data());

      buffer.size_adjust(pos);

      bool result = (pos > 0);

      U_RETURN(result);
      }

   static bool decode(const UString& s, UString& buffer)
      { return decode(U_STRING_TO_PARAM(s), buffer); }
};

#endif
