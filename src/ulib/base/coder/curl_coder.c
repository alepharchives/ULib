// ============================================================================
//
// = LIBRARY
//    ulibase - c++ library
//
// = FILENAME
//    curl_coder.c
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

/*
#define DEBUG_DEBUG
*/

#include <ulib/base/utility.h>
#include <ulib/base/coder/url.h>

#include <ctype.h>
#include <stdlib.h>

/*
Synopsis: Performs URL escaping on a string. This works as follows: all characters except alphanumerics
          and spaces are converted into the 3-byte sequence "%xx" where xx is the character's hexadecimal
          value; spaces are replaced by '+'. Line breaks are stored as "%0D%0A", where a 'line break' is
          any one of: "\n", "\r", "\n\r", or "\r\n"
*/

// extra_enc_chars -> " *'%()<>@,;:\\\"/[]?="
// -------------------------------------------------------------------------------------------------------------------
// RFC2231 encoding (which in HTTP is mostly used for giving UTF8-encoded filenames in the Content-Disposition header)

uint32_t u_url_encode(const unsigned char* restrict input, uint32_t len, unsigned char* restrict result, const char* restrict extra_enc_chars)
{
   uint32_t i;
   unsigned char ch;
   unsigned char* restrict r = result;

   U_INTERNAL_TRACE("u_url_encode(%.*s,%u,%p,%s)", U_min(len,128), input, len, result, extra_enc_chars)

   U_INTERNAL_ASSERT_POINTER(input)

   for (i = 0; i < len; ++i)
      {
      ch = input[i];

      U_INTERNAL_PRINT("ch = %c u_uri_encoded_char[ch] = %d", ch, u_uri_encoded_char[ch])

      if (ch == ' ')
         {
         *r++ = '+';
         }
      else if ((u_uri_encoded_char[ch] & u_uri_encoded_char_mask) ||
               (extra_enc_chars && strchr(extra_enc_chars, ch)))
         {
         *r++ = '%';

         /* char in C++ can be more than 8bit */

         *r++ = u_hex_upper[(ch >> 4) & 0x0F];
         *r++ = u_hex_upper[ ch % 16];
         }
      else
         {
         *r++ = ch;
         }
      }

   *r = 0;

   return (r - result);
}

uint32_t u_url_decode(const char* restrict input, uint32_t len, unsigned char* restrict result, bool no_line_break)
{
   uint32_t i;
   unsigned char* restrict r = result;
   char ch, hex[3]  = { '\0', '\0', '\0' };

   U_INTERNAL_TRACE("u_url_decode(%.*s,%u,%p,%lu,%d)", U_min(len,128), input, len, result, no_line_break)

   U_INTERNAL_ASSERT_POINTER(input)

   for (i = 0; i < len; ++i)
      {
      ch = input[i];

      switch (ch)
         {
         case '+': *r++ = ' '; break;

         case '%':
            {
            /* Line breaks are stored as "%0D%0A", where a 'line break' is any one of: "\n", "\r", "\n\r", or "\r\n" */

            if (no_line_break     &&
                input[i+1] == '0' &&
                input[i+2] == 'D' &&
                input[i+3] == '%' &&
                input[i+4] == '0' &&
                input[i+5] == 'A')
               {
               i += 5;

               continue;
               }

            /* NOTE: wrong input can finish with "...%" giving buffer overflow, cut string here */

            if (input[i+1])
               {
               hex[0] = input[++i];

               if (input[i+1]) hex[1] = input[++i];
               else            hex[1] = 0;
               }
            else
               {
               hex[0] = hex[1] = 0;
               }

            *r++ = (unsigned char) u_hex2int((const char* restrict)hex, 2);
         // *r++ = (unsigned char) strtol((const char*)hex, NULL, 16);
            }
         break;

         default:
            *r++ = ch;
         break;
         }
      }

   *r = 0;

   return (r - result);
}
