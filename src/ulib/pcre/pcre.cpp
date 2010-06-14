// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    pcre.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/pcre/pcre.h>

UPCRE::UPCRE(const UString& expression, int flags) : _expression(expression)
{
   U_TRACE_REGISTER_OBJECT(0, UPCRE, "%.*S,%u", U_STRING_TO_TRACE(expression), flags)

     case_t = ((flags & PCRE_CASELESS) != 0);
   global_t = ((flags & PCRE_GLOBAL)   != 0);

   if (global_t) flags -= PCRE_GLOBAL; /* remove internal flag before feeding _flags to pcre */

   _flags = flags;

      zero();
   compile();
}

UPCRE::~UPCRE()
{
   U_TRACE_UNREGISTER_OBJECT(0, UPCRE)

   /* avoid deleting of uninitialized pointers */

   if (p_pcre)       pcre_free(p_pcre);
   if (p_pcre_extra) pcre_free(p_pcre_extra);

   if (sub_vec) U_FREE_N(sub_vec, sub_len, int);

   if (stringlist)
      {
      pcre_free_substring_list(stringlist);

      U_INTERNAL_ASSERT_POINTER(resultset)

      delete resultset;
      }
}

U_NO_EXPORT const char* UPCRE::status(int num)
{
   U_TRACE(0, "UPCRE::status(%d)", num)

   const char* descr;

   switch (num)
      {
      case  0: descr = "PCRE_OK";                 break;
      case -1: descr = "PCRE_ERROR_NOMATCH";      break;
      case -2: descr = "PCRE_ERROR_NULL";         break;
      case -3: descr = "PCRE_ERROR_BADOPTION";    break;
      case -4: descr = "PCRE_ERROR_BADMAGIC";     break;
      case -5: descr = "PCRE_ERROR_UNKNOWN_NODE"; break;
      case -6: descr = "PCRE_ERROR_NOMEMORY";     break;
      case -7: descr = "PCRE_ERROR_NOSUBSTRING";  break;

      default: descr = "Code unknown"; break;
      }

   static char buffer[128];

   (void) sprintf(buffer, "(%d, %s)", num, descr);

   U_RETURN(buffer);
}

/* compile the expression */

void UPCRE::compile()
{
   U_TRACE(0, "UPCRE::compile()")

   char* ptr = (char*) _expression.c_str();

   U_INTERNAL_ASSERT_POINTER(ptr)

   p_pcre = pcre_compile(ptr, _flags, (const char**)(&err_str), &erroffset, tables);

#ifdef DEBUG
   if (p_pcre == NULL) /* umh, that's odd, the parser should not fail at all */
      {
      U_INTERNAL_DUMP("pcre_compile(%S,%d,%p,%p,%S) failed: %S at: %S", ptr, _flags, &err_str, &erroffset, tables, err_str, ptr + erroffset)
      }
#endif

   U_INTERNAL_ASSERT_POINTER(p_pcre)

   /* calculate the number of substrings we are willing to catch */

   int where;

#ifdef DEBUG
   int info =
#endif
   pcre_fullinfo(p_pcre, p_pcre_extra, PCRE_INFO_CAPTURECOUNT, &where);

   U_DUMP("status() = %S", status(info))

   U_INTERNAL_ASSERT_EQUALS(info,0)

   sub_len = (where+2) * 3; /* see "man pcre" for the exact formula */

   U_INTERNAL_DUMP("sub_len = %d", sub_len)

   reset();
}

UString UPCRE::getMatch(int pos)
{
   U_TRACE(0, "UPCRE::getMatch(%d)", pos)

   U_INTERNAL_ASSERT_POINTER(resultset)
   U_INTERNAL_ASSERT_RANGE(0,pos,num_matches)

   UString result = resultset->at(pos);

   U_RETURN_STRING(result);
}

bool UPCRE::search(const char* stuff, uint32_t stuff_len, int offset, int options)
{
   U_TRACE(0, "UPCRE::search(%.*S,%u,%d,%d)", stuff_len, stuff, stuff_len, offset, options)

   U_INTERNAL_ASSERT_POINTER(p_pcre)

   reset();

   if (sub_vec)  U_FREE_N(sub_vec, sub_len, int);
       sub_vec = U_MALLOC_N(       sub_len, int);

   if (stuff_len == 0) stuff_len = u_strlen(stuff);

   int num = pcre_exec(p_pcre, p_pcre_extra, stuff, stuff_len, offset, options, sub_vec, sub_len);

   U_INTERNAL_DUMP("pcre_exec(%p,%p,%.*S,%u,%d,%d,%p,%d) = %d", p_pcre, p_pcre_extra, stuff_len, stuff, stuff_len, offset, options, sub_vec, sub_len, num)

   if (num <= 0) /*  <0 no match at all;
                    ==0 vector too small, there were too many substrings in stuff */
      {
      U_RETURN(false);
      }

   did_match = true;

   if (num == 1) /* we had a match, but without substrings */
      {
      num_matches = 0;
      }
   else if (num > 1) /* we had matching substrings */
      {
      num_matches = num - 1;

      if (stringlist)
         {
         U_INTERNAL_ASSERT_POINTER(resultset)

         resultset->clear();

         pcre_free_substring_list(stringlist);
         }
      else
         {
         U_INTERNAL_ASSERT_EQUALS(resultset,0)

         resultset = U_NEW(UVector<UString>);
         }

#  ifdef DEBUG
      int res =
#  endif
      pcre_get_substring_list(stuff, sub_vec, num, &stringlist);

      U_DUMP("status() = %S", status(res))

      U_INTERNAL_ASSERT_EQUALS(res,0)

      for (int i = 1; i < num; ++i)
         {
         UString str(stringlist[i]);

         resultset->push_back(str);
         }
      }

   U_RETURN(true);
}

uint32_t UPCRE::split(UVector<UString>& vec, const UString& piece, int limit, int start_offset, int end_offset)
{
   U_TRACE(0, "UPCRE::split(%p,%.*S,%d,%d,%d)", &vec, U_STRING_TO_TRACE(piece), limit, start_offset, end_offset)

   uint32_t n = vec.size();

   if (_expression.length() == 1) /* _expression will be used as delimiter */
      {
      /* use the plain c++ way, ignore the pre-compiled p_pcre */

      char z;
      uint32_t pos;
      uint32_t length = piece.length();
      UString buffer(100U), _delimiter, _piece;

      if (case_t)
         {
         z = u_toupper(_expression[0]);

         for (pos = 0; pos < length; ++pos) _piece.push_back((char)u_toupper(piece[pos]));
         }
      else
         {
         z      = _expression[0];
         _piece = piece;
         }

      for (pos = 0; pos < length; ++pos)
         {
         if (_piece[pos] == z)
            {
            vec.push_back(buffer);

            buffer.setEmpty();
            buffer.duplicate();
            }
         else
            {
            buffer.push_back(piece[pos]);
            }
         }

      if (buffer.empty() == false) vec.push_back(buffer);
      }
   else /* use the regex way */
      {
      if (_expression.first_char() != '(' ||
          _expression.last_char()  != ')' ||
          u_strpend(U_STRING_TO_PARAM(_expression), U_CONSTANT_TO_PARAM("()"), 0) != _expression.c_pointer(_expression.size()-1))
         {
         /* oh, oh - the pre-compiled expression does not contain brackets */

         pcre_free(p_pcre);
         pcre_free(p_pcre_extra);

         p_pcre       = 0;
         p_pcre_extra = 0;

         _expression = "(" + _expression + ")";

         compile();
         }

      int num_pieces = 0, pos = 0, piece_end = 0, piece_start = 0;

      for (;;)
         {
         if (search(piece, pos, 0))
            {
            if (num_matches > 0)
               {
               piece_end   = getMatchStart(0) - 1;
               piece_start = pos;
               pos         = piece_end + 1 + getMatchLength(0);

               ++num_pieces;

               UString junk(piece, piece_start, (piece_end - piece_start)+1);

               if ((limit != 0 && num_pieces < limit) || limit == 0)
                  {
                  if ((start_offset != 0 && num_pieces >= start_offset) || start_offset == 0)
                     {
                     if ((end_offset != 0 && num_pieces <= end_offset) || end_offset == 0)
                        {
                        vec.push_back(junk); /* we are within the allowed range, so just add the grab */
                        }
                     }
                  }
               }
            }
         else
            {
            /* the rest of the UString, there are no more delimiters */

            ++num_pieces;

            UString junk(piece, pos, (piece.length() - pos));

            if ((limit != 0 && num_pieces < limit) || limit == 0)
               {
               if ((start_offset != 0 && num_pieces >= start_offset) || start_offset == 0)
                  {
                  if ((end_offset != 0 && num_pieces <= end_offset) || end_offset == 0)
                     {
                     vec.push_back(junk); /* we are within the allowed range, so just add the grab */
                     }
                  }
               }

            break;
            }
         }
      }

   uint32_t r = vec.size() - n;

   U_RETURN(r);
}

/* replace method */

UString UPCRE::replace(const UString& piece, const UString& with)
{
   U_TRACE(0, "UPCRE::replace(%.*S,%.*S)", U_STRING_TO_TRACE(piece), U_STRING_TO_TRACE(with))

   /*
   certainly we need an anchor, we want to check if the whole arg is in brackets

      braces("^[^\\\\]\\(.*[^\\\\]\\)$");
      perlish: [^\\]\(.*[^\\]\)

   There's no reason, not to add brackets in general.
   It's more comfortable, cause we wants to start with $1 at all, also if we set the whole arg in brackets!
   */

   if (_expression.first_char() != '(' ||
       _expression.last_char()  != ')' ||
       u_strpend(U_STRING_TO_PARAM(_expression), U_CONSTANT_TO_PARAM("()"), 0) != _expression.c_pointer(_expression.size()-1))
      {
      /* oh, oh - the pre-compiled expression does not contain brackets */
      /* recreate the p_pcre* objects to avoid memory leaks */

      pcre_free(p_pcre);
      pcre_free(p_pcre_extra);

      p_pcre       = 0;
      p_pcre_extra = 0;

      _expression = "(" + _expression + ")";

      compile();
      }

   bool bReplaced = false;
   int  iReplaced = -1;
   UString replaced(piece);

   if (search(piece, 0, 0))
      {
      /* we found at least one match */

      // sure we must resolve $1 for ever piece we found especially for "g"
      // so let's just create that var, we resolve it when we needed!

      UString use_with;

      if (global_t == false)
         {
         // here we can resolve vars if option g is not set

         use_with = _replace_vars(with);

         if (matched() &&
             matches() >= 1)
            {
            int len = getMatchEnd() - getMatchStart() + 1;

            replaced.replace(getMatchStart(0), len, use_with);

            bReplaced = true;
            iReplaced = 0;
            }
         }
      else
         {
         /* global replace */

         // in global replace we just need to remember our position
         // so let's initialize it first

         int len, match_pos;

         do {
            // here we need to resolve the vars certainly for every hit.
            // could be different content sometimes!

            use_with = _replace_vars(with);
                                   
            len = getMatchEnd() - getMatchStart() + 1;

            replaced.replace(getMatchStart(0), len, use_with);
                                   
            // Next run should begin after the last char of the stuff we put in the text

            match_pos = ( use_with.length() - len ) + getMatchEnd() + 1;

              bReplaced = true;
            ++iReplaced;
            }
         while (search(replaced, match_pos, 0));
         }
      }

   did_match   = bReplaced;
   num_matches = iReplaced;

   U_RETURN_STRING(replaced);
}

U_NO_EXPORT UString UPCRE::_replace_vars(const UString& piece)
{
   U_TRACE(0, "UPCRE::_replace_vars(%.*S)", U_STRING_TO_TRACE(piece))

   static UPCRE* dollar;

   if (dollar == 0) U_NEW_ULIB_OBJECT(dollar, UPCRE(U_STRING_FROM_CONSTANT("\\${?([0-9]+)}?"), 0));

   UPCRE subsplit;
   int iBracketIndex;
   uint32_t size, pos;
   UVector<UString> splitted;
   UString cstr(U_CAPACITY), first, replaced, sBracketContent, with = piece;

   while (dollar->search(with, 0, 0))
      {
      // let's do some conversion first

      first = dollar->getMatch(0);

      iBracketIndex = first.strtol();

      sBracketContent = getMatch(iBracketIndex);

      U_INTERNAL_DUMP("sBracket[%d] = %.*S", iBracketIndex, U_STRING_TO_TRACE(sBracketContent))

      // now we can split the stuff

      cstr.snprintf("\\${?%.*s}?", U_STRING_TO_TRACE(first));

      subsplit.set(cstr);

      // normally 2 (or more) parts, the one in front of and the other one after "$..."

      replaced.clear();
      splitted.clear();

      size = subsplit.split(splitted, with);

      for (pos = 0; pos < size; ++pos)
         {
         if (pos == (size - 1)) replaced += splitted[pos];
         else                   replaced += splitted[pos] + sBracketContent;
         }

      with = replaced; // well, one part is done
      }

   U_RETURN_STRING(with);
}

// regular expressions that I have found the most useful for day-to-day web programming tasks...
//
// http://immike.net/blog/2007/04/06/5-regular-expressions-every-web-programmer-should-know/

/*
#define U_REGEX_URL "{" \
"\\b" \
"(" \
"(https?)://[-\\w]+(\\.\\w[-\\w]*)+" \
"|" \
 "(?i:[a-z0-9](?:[-a-z0-9]*[a-z0-9])?\\.)+" \
 "(?-i:com\\b" \
    "|edu\\b" \
    "|biz\\b" \
    "|gov\\b" \
    "|in(?:t|fo)\\b" \
    "|mil\\b" \
    "|net\\b" \
    "|org\\b" \
    "|[a-z][a-z]\\.[a-z][a-z]\\b" \
 ")" \
")" \
"(:\\d+)?" \
"(" \
 "/" \
 "[^.!,?;\"\'<>()\\[\\]\\{\\}\\s\\x7F-\\xFF]*" \
   "(" \
     "[.!,?]+[^.!,?;\"\'<>()\\[\\]\\{\\}\\s\\x7F-\\xFF]+" \
   ")*" \
 ")?" \
"}ix"

bool UPCRE::isValidURL(const UString& url)
{
   U_TRACE(0, "UPCRE::isValidURL(%.*S)", U_STRING_TO_TRACE(url))

   static UPCRE* url_mask;

   if (url_mask == 0)
      {
      U_NEW_ULIB_OBJECT(url_mask, UPCRE(U_STRING_FROM_CONSTANT(U_REGEX_URL), 0));

      url_mask->study();
      }

   bool result = url_mask->search(url, 0, 0);

   U_RETURN(result);
}
*/

bool UPCRE::validateUsername(const UString& username)
{
   U_TRACE(0, "UPCRE::validateUsername(%.*S)", U_STRING_TO_TRACE(username))

   static UPCRE* username_mask;

   if (username_mask == 0)
      {
      U_NEW_ULIB_OBJECT(username_mask, UPCRE(U_STRING_FROM_CONSTANT("/^[a-zA-Z0-9_]{3,16}$/"), 0));

      username_mask->study();
      }

   bool result = username_mask->search(username, 0, 0);

   U_RETURN(result);
}

// Matching an XHTML/XML tag with a certain attribute value
// The function tags an attribute, value, input text, and an optional tag name as arguments.
// If no tag name is specified it will match any tag with the specified attribute and attribute value

uint32_t UPCRE::getTag(UVector<UString>& vec, const UString& xml, const char* attr, const char* value, const char* tag)
{
   U_TRACE(0, "UPCRE::getTag(%p,%.*S,%S,%S,%S)", &vec, U_STRING_TO_TRACE(xml), attr, value, tag)

   static UString* cstr;
   static UPCRE* xml_mask;

   if (xml_mask == 0)
      {
      U_NEW_ULIB_OBJECT(cstr,     UString(U_CAPACITY));
      U_NEW_ULIB_OBJECT(xml_mask, UPCRE());

      cstr->snprintf("/<(%s)[^>]*%s\\s*=\\s*([\\'\\\"])%s\\\\2[^>]*>(.*?)<\\/\\\\1>/", tag, attr, value);

      xml_mask->set(*cstr);
      xml_mask->study();
      }

   uint32_t result = xml_mask->split(vec, xml, 0, 0, 0);

   U_RETURN(result);
}

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UPCRE::dump(bool reset) const
{
   *UObjectIO::os << "p_pcre                        " << (void*)p_pcre       << '\n'
                  << "_flags                        " << _flags              << '\n'
                  << "tables                        " << (void*)tables       << '\n'
                  << "case_t                        " << case_t              << '\n'
                  << "sub_len                       " << sub_len             << '\n'
                  << "sub_vec                       " << (void*)sub_vec      << '\n'
                  << "err_str                       " << (void*)err_str      << '\n'
                  << "global_t                      " << global_t            << '\n'
                  << "erroffset                     " << erroffset           << '\n'
                  << "did_match                     " << did_match           << '\n'
                  << "stringlist                    " << (void*)stringlist   << '\n'
                  << "num_matches                   " << num_matches         << '\n'
                  << "p_pcre_extra                  " << (void*)p_pcre_extra << '\n'
                  << "_expression (UString          " << (void*)&_expression << ")\n"
                  << "resultset   (UVector<UString> " << (void*)resultset    << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
