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
 *  \r  CR  carriage return    (\015 13  D)
 *  \n  LF  newline            (\012 10  A) 
 *  \t  HT  horizontal tab     (\011  9  9)
 *  \b  BS  backspace          (\010  8  8) 
 *  \f  FF  formfeed           (\014 12  C) 
 *  \v  VT  vertical tab       (\013 11  B)
 *  \a  BEL                    (\007  7  7)
 *  \e  ESC character          (\033 27 1B)
 *
 *  \u    four-hex-digits (unicode char)
 *  \^C   C = any letter (Control code)
 *  \xDD  number formed of 1-2 hex   digits
 *  \DDD  number formed of 1-3 octal digits
 * ---------------------------------------------------------------------------
 */

uint32_t u_sprintc(char* restrict _buffer, unsigned char c, bool json)
{
   U_INTERNAL_TRACE("u_sprintc(%d,%d)", c, json)

   switch (c)
      {
      case '\r':
         {
         *_buffer++ = '\\';
         *_buffer   = 'r';

         return 2;
         }

      case '\n':
         {
         *_buffer++ = '\\';
         *_buffer   = 'n';

         return 2;
         }

      case '\t':
         {
         *_buffer++ = '\\';
         *_buffer   = 't';

         return 2;
         }

      case '\\':
         {
         *_buffer++ = '\\';
         *_buffer   = '\\';

         return 2;
         }

      case '"':
         {
         *_buffer++ = '\\';
         *_buffer   = '"';

         return 2;
         }

      case '\b':
         {
         *_buffer++ = '\\';
         *_buffer   = 'b';

         return 2;
         }

      case '\f':
         {
         *_buffer++ = '\\';
         *_buffer   = 'f';

         return 2;
         }

      default:
         {
         /*
         if (json &&
             c == '/')
            {
            *_buffer++ = '\\';
            *_buffer   = '/';

            return 2;
            }
         */

         if ((c <  32) ||
             (c > 126))
            {
            char* restrict cp;

            if (json)
               {
               /* \u four-hex-digits (unicode char) */

               *_buffer++ = '\\';
               *_buffer++ = 'u';
               *_buffer++ = '0';
               *_buffer++ = '0';
               *_buffer++ = u_hex_upper[((c >> 4) & 0x0F)];
               *_buffer   = u_hex_upper[( c       & 0x0F)];

               return 6;
               }

            /* \DDD number formed of 1-3 octal digits */

            cp = _buffer + 4;

            *_buffer = '\\';

            do {
               *--cp = (c & 7) + '0';

               c >>= 3;
               }
            while (c);

            while (--cp > _buffer) *cp = '0';

            return 4;
            }

         *_buffer = c;

         return 1;
         }
      }
}

uint32_t u_escape_encode(const unsigned char* restrict inptr, uint32_t len, char* restrict out, uint32_t max_output, bool json)
{
         unsigned char c;
   const unsigned char* restrict inend  = inptr + len;
                  char* restrict outptr = out;
                  char* restrict outend = out + (max_output - 4);

   U_INTERNAL_TRACE("u_escape_encode(%.*s,%u,%p,%u,%d)", U_min(len,128), inptr, len, out, max_output, json)

   U_INTERNAL_ASSERT_POINTER(out)
   U_INTERNAL_ASSERT_POINTER(inptr)

   *outptr++ = '"';

   while (inptr < inend)
      {
      c = *inptr++;

      /* \u four-hex-digits (unicode char) */

      if (json             &&
          c        == '\\' &&
          inptr[0] == 'u') 
         {
         *outptr++ = '\\';
         }
      else
         {
         outptr += u_sprintc(outptr, c, json);
         }

      if (outptr >= outend)
         {
#     ifdef DEBUG
         if (json) U_INTERNAL_ASSERT_MSG(false, "overflow in encoding json string...")
#     endif

         *outptr++ = '.';
         *outptr++ = '.';
         *outptr++ = '.';

         break;
         }
      }

   *outptr++ = '"';
   *outptr   = 0;

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
      case 'r': c = '\r';   break;
      case 'n': c = '\n';   break;
      case 't': c = '\t';   break;
      case 'b': c = '\b';   break;
      case 'f': c = '\f';   break;
      case 'v': c = '\v';   break;
      case 'a': c = '\a';   break;
      case 'e': c = '\033'; break;

      /* check control code */

      case '^': c = u__toupper(*t++) - '@'; break;

      /* check sequenza escape esadecimale */

      case 'x':
         {
         if (u__isxdigit(*t))
            {
                                c =            u__hexc2int(*t++);
            if (u__isxdigit(*t)) c = (c << 4) | u__hexc2int(*t++);
            }
         }
      break;

      /* check sequenza escape ottale */

      case '0': case '1': case '2': case '3':
      case '4': case '5': case '6': case '7':
         {
         c -= '0';

         if (u__isoctal(*t))
            {
                               c = (c << 3) | u__octc2int(*t++);
            if (u__isoctal(*t)) c = (c << 3) | u__octc2int(*t++);
            }
         }
      break;

      /* \u four-hex-digits (unicode char) */

      case 'u':
         {
         U_INTERNAL_ASSERT_EQUALS(t[0],'0')
         U_INTERNAL_ASSERT_EQUALS(t[1],'0')

         t += 2;

         U_INTERNAL_ASSERT(u__isxdigit(t[0]))
         U_INTERNAL_ASSERT(u__isxdigit(t[1]))

         c =            u__hexc2int(*t++);
         c = (c << 4) | u__hexc2int(*t++);
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

         if (len)
            {
            u__memcpy(outptr, inptr, len, __PRETTY_FUNCTION__);

            outptr += len;
            }

         /* \u four-hex-digits (unicode char) */

         if ( inptr[1] == 'u' &&
             (inptr[2] != '0' ||
              inptr[3] != '0'))
            {
            u__memcpy(outptr, inptr, 6, __PRETTY_FUNCTION__);

            outptr += 6;
            inptr  += 6;

            continue;
            }

          inptr    = p + 1;
         *outptr++ = u_escape_decode_ptr((const char** restrict)&inptr); 
         }
      else
         {
         len = inend - inptr;

         if (len)
            {
            u__memcpy(outptr, inptr, len, __PRETTY_FUNCTION__);

            outptr += len;
            }

         break;
         }
      }

   *outptr = 0;

   return (outptr - out);
}
