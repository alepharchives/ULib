// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    tokenizer.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/tokenizer.h>
#include <ulib/utility/services.h>
#include <ulib/container/vector.h>

bool        UTokenizer::group_skip;
uint32_t    UTokenizer::group_len;
uint32_t    UTokenizer::group_len_div_2;
const char* UTokenizer::group;

bool UTokenizer::next(UString& token, char c)
{
   U_TRACE(0, "UTokenizer::next(%p,%C)", &token, c)

   const char* p;

loop:
   U_SKIP_CHAR(s,end,loop,done,c)

   U_DELIMIT_TOKEN_CHAR(s,end,p,c)

   token = str.substr(p, s - p);

   ++s;

   U_RETURN(true);

done:
   U_RETURN(false);
}

// extend the actual token to the next char 'c'... (see PEC_report.cpp)

bool UTokenizer::extend(UString& token, char c)
{
   U_TRACE(0, "UTokenizer::extend(%p,%C)", &token, c)

   const char* p;

loop:
   U_SKIP_CHAR(s,end,loop,done,c)

   U_DELIMIT_TOKEN_CHAR(s,end,p,c)

   p = token.data();

   token = str.substr(p, s - p);

   ++s;

   U_RETURN(true);

done:
   U_RETURN(false);
}

bool UTokenizer::next(UString& token, bool* bgroup)
{
   U_TRACE(1, "UTokenizer::next(%p,%p)", &token, bgroup)

   const char* p  = s;
   uint32_t shift = 1;

   if (bgroup) *bgroup = false;

loop:
   if (delim)
      {
      U_SKIP_EXT(s,end,loop,done,delim)

      U_DELIMIT_TOKEN_EXT(s,end,p,delim)

      goto tok;
      }

   U_SKIP(s,end,loop,done)

   if (group)
      {
      if (U_SYSCALL(memcmp, "%S,%S,%u", s, group, group_len_div_2) == 0)
         {
         p = s + group_len_div_2 - 1;
         s = u_strpend(p, end - p, group, group_len, '\0');

         ++p;

         if (s == 0) s = end;

         if (group_skip)
            {
            s += group_len_div_2;

            goto loop;
            }

         if (bgroup) *bgroup = true;

         shift = group_len_div_2;

         goto tok;
         }
      else if (group_skip)
         {
         // -------------------------------------------------------------------
         // examples:
         // -------------------------------------------------------------------
         // <date>03/11/2005 10:17:46</date>
         // <description>description_556adfbc-0107-5000-ede4-d208</description>
         // -------------------------------------------------------------------

         U_DELIMIT_TOKEN(s,end,p)

         if (s < end)
            {
            const char* x = (char*) memchr(p, group[0], s - p);

            if (x && (U_SYSCALL(memcmp, "%S,%S,%u", x, group, group_len_div_2) == 0))
               {
               s     = x;
               shift = 0;
               }
            }

         goto tok;
         }
      }

   U_DELIMIT_TOKEN(s,end,p)

tok:
   token = str.substr(p, s - p);

   s += shift;

   U_RETURN(true);

done:
   U_RETURN(false);
}

bool UTokenizer::tokenSeen(UString* x)
{
   U_TRACE(0, "UTokenizer::tokenSeen(%.*S)", U_STRING_TO_TRACE(*x))

   U_INTERNAL_DUMP("s = %.*S", end - s, s)

   skipSpaces();

   if (s < end)
      {
      uint32_t sz = x->size();

      if (memcmp(s, x->data(), sz) == 0)
         {
         s += sz;

         U_RETURN(true);
         }
      }

   U_RETURN(false);
}

UString UTokenizer::getTokenQueryParser()
{
   U_TRACE(0, "UTokenizer::getTokenQueryParser()")

   skipSpaces();

   const char* p = s++;

   if (*p == '"')
      {
      while (s < end && *s++ != '"') {}
      }
   else
      {
      while (s < end &&                // (isalnum(*s) || *s == '_'))
             (u_isspace(*s) == false &&
              *s != '(' &&
              *s != ')'))
         {
         ++s;
         }
      }

   UString token = str.substr(p, s - p);

   U_RETURN_STRING(token);
}

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UTokenizer::dump(bool reset) const
{
   *UObjectIO::os << "s                           " << (void*)s   << '\n'
                  << "end                         " << (void*)end << '\n'
                  << "group                       ";

   char buffer[32];

   UObjectIO::os->write(buffer, u_snprintf(buffer, sizeof(buffer), "%S", group));

   *UObjectIO::os << '\n'
                  << "delim                       ";

   UObjectIO::os->write(buffer, u_snprintf(buffer, sizeof(buffer), "%S", delim));

   *UObjectIO::os << '\n'
                  << "group_skip                  " << group_skip  << '\n'
                  << "str       (UString          " << (void*)&str << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
