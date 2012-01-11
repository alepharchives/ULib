// ============================================================================
//
// = LIBRARY
//    ulibase - c++ library
//
// = FILENAME
//    chexdump.c
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

/*
#define DEBUG_DEBUG
*/

#include <ulib/base/utility.h>
#include <ulib/base/coder/hexdump.h>

uint32_t u_hexdump_encode(const unsigned char* restrict input, uint32_t len, unsigned char* restrict result)
{
   uint32_t i;
   unsigned char ch;
   unsigned char* restrict r = result;

   U_INTERNAL_TRACE("u_hexdump_encode(%.*s,%u,%p)", U_min(len,128), input, len, result)

   U_INTERNAL_ASSERT_POINTER(input)

   for (i = 0; i < len; ++i)
      {
      ch = input[i];

      *r++ = u_hex_lower[(ch >> 4) & 0x0f];
      *r++ = u_hex_lower[(ch     ) & 0x0f];
      }

   *r = 0;

   return (r - result);
}

uint32_t u_hexdump_decode(const char* restrict input, uint32_t len, unsigned char* restrict result)
{
   int c;
   int32_t i = 0;
   unsigned char* restrict r = result;

   U_INTERNAL_TRACE("u_hexdump_decode(%.*s,%u,%p,%lu)", U_min(len,128), input, len, result)

   U_INTERNAL_ASSERT_POINTER(input)

   while (i < (int32_t)len)
      {
      /*
      U_INTERNAL_ASSERT(u_isxdigit(input[i]))
      U_INTERNAL_ASSERT(u_isxdigit(input[i+1]))
      */

      c =            u_hexc2int(input[i++]);
      c = (c << 4) | u_hexc2int(input[i++]);

      *r++ = (unsigned char) c;
      }

   *r = 0;

   return (r - result);
}
