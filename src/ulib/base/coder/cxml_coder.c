// ============================================================================
//
// = LIBRARY
//    ulibase - c++ library
//
// = FILENAME
//    cxml_coder.c
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/base/utility.h>
#include <ulib/base/coder/xml.h>

/**
  Encoding to escape and unescape character to valid CDATA characters.

     Character      XML escape sequence        name

       '"'          "&quot;"                   quote
       '&'          "&amp;"                    amp
       '\''         "&apos;"                   apostrophe
       '<'          "&lt;"                     lower than
       '>'          "&gt;"                     greater than
*/

static inline unsigned char* u_set_quot(unsigned char* r)
{
   *r++ = '&';
   *r++ = 'q';
   *r++ = 'u';
   *r++ = 'o';
   *r++ = 't';
   *r++ = ';';

   return r;
}

static inline unsigned char* u_set_amp(unsigned char* r)
{
   *r++ = '&';
   *r++ = 'a';
   *r++ = 'm';
   *r++ = 'p';
   *r++ = ';';

   return r;
}

static inline unsigned char* u_set_apos(unsigned char* r)
{
   *r++ = '&';
   *r++ = 'a';
   *r++ = 'p';
   *r++ = 'o';
   *r++ = 's';
   *r++ = ';';

   return r;
}

static inline unsigned char* u_set_lt(unsigned char* r)
{
   *r++ = '&';
   *r++ = 'l';
   *r++ = 't';
   *r++ = ';';

   return r;
}

static inline unsigned char* u_set_gt(unsigned char* r)
{
   *r++ = '&';
   *r++ = 'g';
   *r++ = 't';
   *r++ = ';';

   return r;
}

uint32_t u_xml_encode(const unsigned char* input, uint32_t len, unsigned char* result)
{
         unsigned char ch;
         unsigned char* r   = result;
   const unsigned char* end = input + len;

   U_INTERNAL_TRACE("u_xml_encode(%.*s,%u,%p)", len, input, len, result)

   U_INTERNAL_ASSERT_POINTER(input)

   while (input < end)
      {
      ch = *input++;

      if (u_isalnum(ch) || strchr("\"&'<>", ch) == 0) *r++ = ch;
      else
         {
         switch (ch)
            {
            case '&':
               {
               /* skip if already encoded... hexadecimal or decimal numerical entities and named entities (&amp; &lt; &gt; &quot; &apos;) */

               if (        *input == '#'    ||
                   U_STRNEQ(input, "quot;") ||
                   U_STRNEQ(input, "amp;")  ||
                   U_STRNEQ(input, "lt;")   ||
                   U_STRNEQ(input, "gt;")   ||
                   U_STRNEQ(input, "apos;"))
                  {
                  *r++ = '&';

                  while ((*r = *input) != ';')
                     {
                     ++r;
                     ++input;
                     }
                  }
               else
                  {
                  r = u_set_amp(r);
                  }
               }
            break;

            case '"':  r  = u_set_quot(r); break;
            case '\'': r  = u_set_apos(r); break;
            case '<':  r  = u_set_lt(r);   break;
            case '>':  r  = u_set_gt(r);   break;
            default: *r++ = ch;            break;
            }
         }
      }

   *r = 0;

   return (r - result);
}

uint32_t u_xml_decode(const char* input, uint32_t len, unsigned char* result)
{
                  char ch;
         unsigned char* r   = result;
   const          char* end = input + len;

   U_INTERNAL_TRACE("u_xml_decode(%.*s,%u,%p,%lu)", len, input, len, result)

   U_INTERNAL_ASSERT_POINTER(input)

   while (input < end)
      {
      ch = *input++;

      if (ch != '&') *r++ = ch;
      else
         {
         /* check for hexadecimal or decimal numerical entities and named entities (&amp; &lt; &gt; &quot; &apos;) */

         if (U_STRNEQ(input, "amp;"))        /* '&' <-> &amp; */
            {
            *r++   = '&';
            input += U_CONSTANT_SIZE("amp;");
            }
         else if (U_STRNEQ(input, "lt;"))    /* '<' <-> &lt; */
            {
            *r++   = '<';
            input += U_CONSTANT_SIZE("lt;");
            }
         else if (U_STRNEQ(input, "gt;"))    /* '>' <-> &gt; */
            {
            *r++   = '>';
            input += U_CONSTANT_SIZE("gt;");
            }
         else if (U_STRNEQ(input, "quot;"))  /* '"' <-> &quot; */
            {
            *r++   = '"';
            input += U_CONSTANT_SIZE("quot;");
            }
         else if (U_STRNEQ(input, "apos;"))  /* '\'' <-> &apos; */
            {
            *r++   = '\'';
            input += U_CONSTANT_SIZE("apos;");
            }
         else if (*input == '#')
            {
            /*
            union uuarg {
                              char* p;
               const          char* cp;
               const unsigned char* cup;
            };

            union uuarg i = { ++input };

            *r++ = (unsigned char) strtol(i.cp, &i.p, 0);
            */

            ++input;

            *r++ = (unsigned char) strtol((const char*)input, (char**)&input, 0);

            ++input;
            }
         else
            {
            *r++ = ch;
            }
         }
      }

   *r = 0;

   return (r - result);
}
