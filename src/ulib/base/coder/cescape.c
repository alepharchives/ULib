// ============================================================================
//
// = LIBRARY
//    ulibase - c++ library
//
// = FILENAME
//    cescape.c
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/base/utility.h>
#include <ulib/base/coder/escape.h>

/* Encode-Decode escape sequences into a buffer, the following are recognized:
 * ---------------------------------------------------------------------------
 *  \0  NUL
 *  \b  BS  backspace          (\010  8  8) 
 *  \t  HT  horizontal tab     (\011  9  9)
 *  \n  LF  newline            (\012 10  A) 
 *  \v  VT  vertical tab       (\013 11  B)
 *  \f  FF  formfeed           (\014 12  C) 
 *  \r  CR  carriage return    (\015 13  D)
 *  \e  ESC character          (\033 27 1B)
 *
 *  \^C   C = any letter (Control code)
 *  \xDD  number formed of 1-2 hex   digits
 *  \DDD  number formed of 1-3 octal digits
 * ---------------------------------------------------------------------------
 */

uint32_t u_sprintc(char* restrict buffer, unsigned char c)
{
   U_INTERNAL_TRACE("u_sprintc(%d)", c)

   switch (c)
      {
      case '\r':
         {
         *buffer++ = '\\';
         *buffer   = 'r';

         return 2;
         }

      case '\n':
         {
         *buffer++ = '\\';
         *buffer   = 'n';

         return 2;
         }

      case '\t':
         {
         *buffer++ = '\\';
         *buffer   = 't';

         return 2;
         }

      case '\b':
         {
         *buffer++ = '\\';
         *buffer   = 'b';

         return 2;
         }

      case '\\':
         {
         *buffer++ = '\\';
         *buffer   = '\\';

         return 2;
         }

      case '"':
         {
         *buffer++ = '\\';
         *buffer   = '"';

         return 2;
         }

      default:
         {
         if ((c <  32) ||
             (c > 126))
            {
            char* restrict cp = buffer + 4;

            *buffer = '\\';

            do {
               *--cp = (c & 7) + '0';

               c >>= 3;
               }
            while (c);

            while (--cp > buffer) *cp = '0';

            return 4;
            }
         else
            {
            *buffer = c;

            return 1;
            }
         }
      }
}

uint32_t u_escape_encode(const unsigned char* restrict inptr, uint32_t len, char* restrict out, uint32_t max_output)
{
         unsigned char c;
   const unsigned char* restrict inend  = inptr + len;
                  char* restrict outptr = out;
                  char* restrict outend = out + (max_output - 4);

   U_INTERNAL_TRACE("u_escape_encode(%.*s,%u,%p,%u)", U_min(len,128), inptr, len, out, max_output)

   U_INTERNAL_ASSERT_POINTER(out)
   U_INTERNAL_ASSERT_POINTER(inptr)

   *outptr++ = '"';

   while (inptr < inend)
      {
      c = *inptr++;

      outptr += u_sprintc(outptr, c);

      if (outptr >= outend)
         {
         *outptr++ = '.';
         *outptr++ = '.';
         *outptr++ = '.';

         break;
         }
      }

   *outptr++ = '"';

   return (outptr - out);
}

/* the s pointer is advanced past the escape sequence */

unsigned char u_escape_decode_ptr(const char** restrict s)
{
   const char* restrict t = *s;
   int c                  = *t++;

   U_INTERNAL_TRACE("u_escape_decode_ptr(%s)", *s)

   switch (c)
      {
      case 'b': c = '\b';   break;
      case 't': c = '\t';   break;
      case 'n': c = '\n';   break;
      case 'v': c = '\v';   break;
      case 'f': c = '\f';   break;
      case 'r': c = '\r';   break;
      case 'e': c = '\033'; break;

      /* check control code */

      case '^': c = u_toupper(*t++) - '@'; break;

      /* check sequenza escape esadecimale */

      case 'x':
         {
         if (u_isxdigit(*t))
            {
                                c =            u_hexc2int(*t++);
            if (u_isxdigit(*t)) c = (c << 4) | u_hexc2int(*t++);
            }
         }
      break;

      /* check sequenza escape ottale */

      case '0': case '1': case '2': case '3':
      case '4': case '5': case '6': case '7':
         {
         c -= '0';

         if (u_isoctal(*t))
            {
                               c = (c << 3) | u_octc2int(*t++);
            if (u_isoctal(*t)) c = (c << 3) | u_octc2int(*t++);
            }
         }
      break;
      }

   *s = t;

   return (unsigned char)c;
}

uint32_t u_escape_decode(const char* restrict inptr, uint32_t len, unsigned char* restrict out)
{
   char* p;
   unsigned char* restrict outptr = out;
   const    char* restrict inend  = inptr + len;

   U_INTERNAL_TRACE("u_escape_decode(%.*s,%u,%p)", U_min(len,128), inptr, len, out)

   U_INTERNAL_ASSERT_POINTER(out)
   U_INTERNAL_ASSERT_POINTER(inptr)

   while (inptr < inend)
      {
      p = (char* restrict) memchr(inptr, '\\', inend - inptr);

      if (p)
         {
         len = p - inptr;

         (void) memcpy(outptr, inptr, len);

         inptr   = p + 1;
         outptr += len;

         *outptr++ = u_escape_decode_ptr((const char** restrict)&inptr); 
         }
      else
         {
         len = inend - inptr;

         (void) memcpy(outptr, inptr, len);

         outptr += len;

         break;
         }
      }

   *outptr = 0;

   return (outptr - out);
}
