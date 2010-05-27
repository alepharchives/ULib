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

   while (s < end)
      {  
      // skip char delimiter

      if (*s == c)
         {
         ++s;

         continue;
         }

      // delimit token with char delimiter

      p = s;
      s = (const char*) memchr(s, c, end - s);

      if (s == 0) s = end;

      token = str.substr(p, s - p);

      ++s;

      U_RETURN(true);
      }

   U_RETURN(false);
}

// extend the actual token to the next char 'c'... (see PEC_report.cpp)

bool UTokenizer::extend(UString& token, char c)
{
   U_TRACE(0, "UTokenizer::extend(%p,%C)", &token, c)

   const char* p;

   while (s < end)
      {  
      // skip char delimiter

      if (*s == c)
         {
         ++s;

         continue;
         }

      // delimit token with char delimiter

      p = token.data();
      s = (const char*) memchr(s, c, end - s);

      if (s == 0) s = end;

      token = str.substr(p, s - p);

      ++s;

      U_RETURN(true);
      }

   U_RETURN(false);
}

bool UTokenizer::next(UString& token, bool* bgroup)
{
   U_TRACE(1, "UTokenizer::next(%p,%p)", &token, bgroup)

   const char* p  = s;
   uint32_t shift = 1;

   if (bgroup) *bgroup = false;

   while (s < end)
      {
      if (delim)
         {
         s = u_delimit_token(s, &p, end, delim, 0);

         goto tok;
         }

      s = u_skip(s, end, 0, 0);

      if (s == end) break;

      if (group)
         {
         if (U_SYSCALL(memcmp, "%S,%S,%u", s, group, group_len_div_2) == 0)
            {
            p = s + group_len_div_2 - 1;
            s = u_strpend(p, end - p, group, group_len, '\0');

            ++p;

            if (s == 0) s = end;

            U_INTERNAL_DUMP("p = %.*S s = %.*S", s - p, p, end - s, s)

            if (group_skip)
               {
               s += group_len_div_2;

               continue;
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

            s = u_delimit_token(s, &p, end, 0, 0);

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

      s = u_delimit_token(s, &p, end, 0, 0);

tok:
      token = str.substr(p, s - p);

      s += shift;

      U_RETURN(true);
      }

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
