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
#include <ulib/utility/string_ext.h>

UPCRE* UPCRE::dollar;
UPCRE* UPCRE::xml_mask;
UPCRE* UPCRE::url_mask;
UPCRE* UPCRE::username_mask;

U_NO_EXPORT bool UPCRE::checkBrackets()
{
   U_TRACE(1, "UPCRE::checkBrackets()")

   /*
   certainly we need an anchor, we want to check if the whole arg is in brackets

      braces("^[^\\\\]\\(.*[^\\\\]\\)$");
      perlish: [^\\]\(.*[^\\]\)

   There's no reason, not to add brackets in general.
   It's more comfortable, cause we wants to start with $1 at all, also if we set the whole arg in brackets!
   */

   if (UStringExt::isDelimited(_expression, "()") == false)
      {
      _expression = '(' + _expression + ')';

      /* recreate the p_pcre* objects to avoid memory leaks */

      if (p_pcre)
         {
         U_SYSCALL_VOID(pcre_free, "%p", p_pcre);

         p_pcre = 0;
         }

      if (p_pcre_extra)
         {
         U_SYSCALL_VOID(pcre_free, "%p", p_pcre_extra);

         p_pcre_extra = 0;
         }

      compile(0);

      U_RETURN(true);
      }

   U_RETURN(false);
}

U_NO_EXPORT void UPCRE::zero(int flags)
{
   U_TRACE(0, "UPCRE::zero(%d)", flags)

   p_pcre       = 0;
   sub_vec      = 0;
   resultset    = 0;
   stringlist   = 0;
   p_pcre_extra = 0;

   if (flags)
      {
       global_t = ((flags & PCRE_GLOBAL)      != 0);
      replace_t = ((flags & PCRE_FOR_REPLACE) != 0);

      /* remove internal flag before feeding _flags to pcre */

      if ( global_t) flags -= PCRE_GLOBAL;
      if (replace_t) flags -= PCRE_FOR_REPLACE;
      }
   else
      {
      replace_t = global_t = false;
      }

   _flags = flags;
}

UPCRE::UPCRE()
{
   U_TRACE_REGISTER_OBJECT(0, UPCRE, "")

   zero(0);
}

UPCRE::UPCRE(const UString& expression, int flags)
{
   U_TRACE_REGISTER_OBJECT(0, UPCRE, "%.*S,%u", U_STRING_TO_TRACE(expression), flags)

   set(expression, flags);
}

UPCRE::UPCRE(const UString& expression, const char* flags)
{
   U_TRACE_REGISTER_OBJECT(0, UPCRE, "%.*S,%S", U_STRING_TO_TRACE(expression), flags)

   set(expression, flags);
}

UPCRE::~UPCRE()
{
   U_TRACE_UNREGISTER_OBJECT(0, UPCRE)

   clear();
}

void UPCRE::set(const UString& expression, int flags)
{
   U_TRACE(0, "UPCRE::set(%.*S,%d)", U_STRING_TO_TRACE(expression), flags)

   U_ASSERT_EQUALS(expression.empty(), false)

   _expression = expression;

   zero(flags);

   bool compiled = (replace_t && checkBrackets());

   if (compiled == false) compile(0);
}

void UPCRE::set(const UString& expression, const char* flags)
{
   U_TRACE(0, "UPCRE::set(%.*S,%S)", U_STRING_TO_TRACE(expression), flags)

   U_ASSERT_EQUALS(expression.empty(), false)

   _expression = expression;

   zero(0);

   for (int i = 0; flags[i] != '\0'; ++i)
      {
      switch (flags[i])
         {
         case 'i':   _flags |= PCRE_CASELESS;  break;
         case 'm':   _flags |= PCRE_MULTILINE; break;
         case 's':   _flags |= PCRE_DOTALL;    break;
         case 'x':   _flags |= PCRE_EXTENDED;  break;
         case 'g':  global_t = true;           break;
         case 'r': replace_t = true;           break;
         }
      }

   compile(0);
}

void UPCRE::clear()
{
   U_TRACE(0, "UPCRE::clear()")

   /* avoid deleting of uninitialized pointers */

   if (p_pcre)
      {
      U_SYSCALL_VOID(pcre_free, "%p", p_pcre);
                                      p_pcre = 0;
      }

   if (p_pcre_extra)
      {
      U_SYSCALL_VOID(pcre_free, "%p", p_pcre_extra);
                                      p_pcre_extra = 0;
      }

   if (sub_vec)
      {
      U_FREE_N(sub_vec, sub_len, int);
               sub_vec = 0;
      }

   if (stringlist)
      {
      U_SYSCALL_VOID(pcre_free_substring_list, "%p", stringlist);
                                                     stringlist = 0;

      U_INTERNAL_ASSERT_POINTER(resultset)

      delete resultset;
             resultset = 0;
      }

   _expression.clear();
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

void UPCRE::compile(const unsigned char* tables) /* locale tables */
{
   U_TRACE(1, "UPCRE::compile(%p)", tables)

   int erroffset;
   const char* err_str;
   char* ptr = (char*) _expression.c_str();

   U_INTERNAL_ASSERT_POINTER(ptr)

   p_pcre = (pcre*) U_SYSCALL(pcre_compile, "%S,%d,%p,%p,%S", ptr, _flags, &err_str, &erroffset, tables);

#ifdef DEBUG
   if (p_pcre == 0) U_INTERNAL_DUMP("pcre_compile() failed: %S at: %S", err_str, ptr + erroffset)
#endif

   U_INTERNAL_ASSERT_POINTER(p_pcre)

   /* calculate the number of substrings we are willing to catch */

   int where, info = U_SYSCALL(pcre_fullinfo, "%p,%p,%d,%p", p_pcre, p_pcre_extra, PCRE_INFO_CAPTURECOUNT, &where);

   U_DUMP("status() = %S", status(info))

   U_INTERNAL_ASSERT_EQUALS(info,0)

   sub_len = (where+2) * 3; /* see "man pcre" for the exact formula */

   U_INTERNAL_DUMP("sub_len = %d", sub_len)

   reset();
}

void UPCRE::study(int options)
{
   U_TRACE(1, "UPCRE::study(%d)", options)

   const char* err_str;

   p_pcre_extra = (pcre_extra*) U_SYSCALL(pcre_study, "%p,%d,%p", p_pcre, options, &err_str);

#ifdef DEBUG
   if (p_pcre_extra == 0 && err_str) U_INTERNAL_DUMP("pcre_study() failed: %S", err_str)
#endif
}

bool UPCRE::search(const char* stuff, uint32_t stuff_len, int offset, int options, bool bresultset)
{
   U_TRACE(1, "UPCRE::search(%.*S,%u,%d,%d,%b)", stuff_len, stuff, stuff_len, offset, options, bresultset)

   U_INTERNAL_ASSERT_POINTER(p_pcre)

   reset();

   if (sub_vec)  U_FREE_N(sub_vec, sub_len, int);
       sub_vec = U_MALLOC_N(       sub_len, int);

   if (stuff_len == 0) stuff_len = u__strlen(stuff);

   int num = U_SYSCALL(pcre_exec, "%p,%p,%S,%u,%d,%d,%p,%d", p_pcre, p_pcre_extra, stuff, stuff_len, offset, options, sub_vec, sub_len);

   /* <  0 no match at all;
      == 0 vector too small, there were too many substrings in stuff */

   if (num <= 0) U_RETURN(false);

   did_match = true;

        if (num == 1) num_matches = 0; /* we had a match, but without substrings */
   else if (num >  1)                  /* we had matching substrings */
      {
      int i;
#ifdef DEBUG
      int res;
#endif

      num_matches = num - 1;

      if (stringlist)
         {
         U_SYSCALL_VOID(pcre_free_substring_list, "%p", stringlist);
                                                        stringlist = 0;

         U_INTERNAL_ASSERT_POINTER(resultset)

         delete resultset;
                resultset = 0;
         }

      if (bresultset == false) goto end;

#ifdef DEBUG
      res =
#endif
      U_SYSCALL(pcre_get_substring_list, "%S,%p,%d,%p", stuff, sub_vec, num, &stringlist);

      U_DUMP("status() = %S", status(res))

      U_INTERNAL_ASSERT_EQUALS(res,0)

      U_DUMP("getMatchStart() = %u getMatchEnd() = %u", getMatchStart(), getMatchEnd())

      U_INTERNAL_ASSERT_EQUALS(resultset,0)

      resultset = U_NEW(UVector<UString>);

      for (i = 1; i < num; ++i)
         {
         UString str(stringlist[i]);

         resultset->push_back(str);

         U_INTERNAL_DUMP("resultset[%d] = (%u) %.*S", i-1, str.size(), U_STRING_TO_TRACE(str))

         U_DUMP("getMatchStart(%d) = %u getMatchEnd(%d) = %u getMatchLength(%d) = %u",
                  i-1, getMatchStart(i-1), i-1, getMatchEnd(i-1), i-1, getMatchLength(i-1))

         U_ASSERT_EQUALS(str.size(), getMatchLength(i-1))
         }
      }

end:
   U_INTERNAL_DUMP("num_matches = %d", num_matches)

   U_RETURN(true);
}

uint32_t UPCRE::split(UVector<UString>& vec, const UString& piece, int limit, int start_offset, int end_offset)
{
   U_TRACE(0, "UPCRE::split(%p,%.*S,%d,%d,%d)", &vec, U_STRING_TO_TRACE(piece), limit, start_offset, end_offset)

   int num_pieces, piece_start, piece_end;
   uint32_t r, pos, sz, n = vec.size(), length = piece.size();

   if (_expression.size() == 1) /* _expression will be used as delimiter */
      {
      /* use the plain c++ way, ignore the pre-compiled p_pcre */

      char z;
      UString buffer(100U), _delimiter, _piece;

      if (_flags & PCRE_CASELESS)
         {
         z = u_toupper(_expression.first_char());

         for (pos = 0; pos < length; ++pos) _piece.push_back(u_toupper(piece.c_char(pos)));
         }
      else
         {
         z      = _expression.first_char();
         _piece = piece;
         }

      for (pos = 0; pos < length; ++pos)
         {
         if (_piece.c_char(pos) == z)
            {
            vec.push_back(buffer);

            buffer.setEmpty();
            buffer.duplicate();
            }
         else
            {
            buffer.push_back(piece.c_char(pos));
            }
         }

      if (buffer.empty() == false) vec.push_back(buffer);

      goto end;
      }

   /* use the regex way */

   if (replace_t == false) (void) checkBrackets();

   pos        = 0;
   num_pieces = 0;

   while (search(piece, pos, 0, false) &&
          matches() > 0) /* we had matching substrings */
      {
      ++num_pieces;

      piece_start = pos;
      piece_end   = getMatchStart(0);
      sz          = piece_end - piece_start;

      // NB: it's ok a null string... (see split in replaceVars())

      U_INTERNAL_DUMP("num_pieces = %d piece_start = %d piece_end = %d sz = %d", num_pieces, piece_start, piece_end, sz)

      if ((       limit == 0 || num_pieces <         limit) &&
          (start_offset == 0 || num_pieces >= start_offset) &&
          (  end_offset == 0 || num_pieces <=   end_offset))
         {
         /* we are within the allowed range, so just add the grab */

         UString junk(piece, piece_start, sz);

         vec.push_back(junk);
         }

      pos = piece_end + getMatchLength(0);

      U_INTERNAL_DUMP("pos = %u", pos)
      }

   /* the rest of the string, there are no more delimiters */

   ++num_pieces;

   if ((       limit == 0 || num_pieces <         limit) &&
       (start_offset == 0 || num_pieces >= start_offset) &&
       (  end_offset == 0 || num_pieces <=   end_offset))
      {
      /* we are within the allowed range, so just add the grab */

      UString junk(piece, pos, length - pos);

      vec.push_back(junk);
      }

end:
   r = vec.size() - n;

   U_RETURN(r);
}

/* replace method */

UString UPCRE::replace(const UString& piece, const UString& with)
{
   U_TRACE(0, "UPCRE::replace(%.*S,%.*S)", U_STRING_TO_TRACE(piece), U_STRING_TO_TRACE(with))

   bool bReplaced = false;
   int iReplaced = -1, len;
   UString replaced(piece), use_with;

   if (replace_t == false) (void) checkBrackets();

   if (search(piece, 0, 0, true) == false) goto end;

   /* we found at least one match */

   U_INTERNAL_DUMP("global_t = %b", global_t)

   if (global_t)
      {
      int match_pos;

      do {
         // here we need to resolve the vars certainly for every hit.
         // could be different content sometimes!

         use_with = replaceVars(with);
                                
         len = getMatchEnd() - getMatchStart() + 1;

         replaced.replace(getMatchStart(0), len, use_with);
                                
         // Next run should begin after the last char of the stuff we put in the text

         match_pos = (use_with.length() - len) + getMatchEnd() + 1;

           bReplaced = true;
         ++iReplaced;
         }
      while (search(replaced, match_pos, 0, true));

      goto end;
      }

   // here we can resolve vars if option g is not set

   use_with = replaceVars(with);

   if (matched() &&
       matches() >= 1)
      {
      len = getMatchEnd() - getMatchStart() + 1;

      replaced.replace(getMatchStart(0), len, use_with);

      bReplaced = true;
      iReplaced = 0;
      }

end:
   did_match   = bReplaced;
   num_matches = iReplaced;

   U_RETURN_STRING(replaced);
}

U_NO_EXPORT UString UPCRE::replaceVars(const UString& piece)
{
   U_TRACE(0, "UPCRE::replaceVars(%.*S)", U_STRING_TO_TRACE(piece))

   if (dollar == 0)
      {
      U_NEW_ULIB_OBJECT(dollar, UPCRE(U_STRING_FROM_CONSTANT("\\${?([0-9]+)}?"), 0));

      dollar->study();
      }

   UPCRE subsplit;
   UVector<UString> splitted;
   uint32_t size, pos, iBracketIndex, last = resultset->size();
   UString cstr(U_CAPACITY), first, replaced, sBracketContent, with = piece;

   while (dollar->search(with, 0, 0, true))
      {
      // let's do some conversion first

      first = dollar->resultset->at(0);

      iBracketIndex = first.strtol();

      // NB: we need this check...

      sBracketContent = resultset->at(iBracketIndex < last ? iBracketIndex : last-1);

      U_INTERNAL_DUMP("sBracket[%d] = %.*S", iBracketIndex, U_STRING_TO_TRACE(sBracketContent))

      // now we can split the stuff

      subsplit.clear();

      cstr.snprintf("(\\${?%.*s}?)", U_STRING_TO_TRACE(first));

      subsplit.set(cstr, PCRE_FOR_REPLACE);

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
// ---------------------------------------------------------------------------------------------
// http://immike.net/blog/2007/04/06/5-regular-expressions-every-web-programmer-should-know/

bool UPCRE::validateUsername(const UString& username)
{
   U_TRACE(0, "UPCRE::validateUsername(%.*S)", U_STRING_TO_TRACE(username))

   if (username_mask == 0)
      {
      U_NEW_ULIB_OBJECT(username_mask, UPCRE(U_STRING_FROM_CONSTANT("(/^[a-zA-Z0-9_]{3,16}$/)"), PCRE_FOR_REPLACE));

      username_mask->study();
      }

   bool result = username_mask->search(username, 0, 0, false);

   U_RETURN(result);
}

// Matching an XHTML/XML tag with a certain attribute value
// The function tags an attribute, value, input text, and an optional tag name as arguments.
// If no tag name is specified it will match any tag with the specified attribute and attribute value

uint32_t UPCRE::getTag(UVector<UString>& vec, const UString& xml, const char* attr, const char* value, const char* tag)
{
   U_TRACE(0, "UPCRE::getTag(%p,%.*S,%S,%S,%S)", &vec, U_STRING_TO_TRACE(xml), attr, value, tag)

   if (xml_mask == 0)
      {
      UString cstr(U_CAPACITY);

      cstr.snprintf("(<(%s)[^>]*%s\\s*=\\s*([\\'\\\"])%s\\\\2[^>]*>(.*?)<\\/\\\\1>)", tag, attr, value);

      U_NEW_ULIB_OBJECT(xml_mask, UPCRE(cstr, PCRE_FOR_REPLACE));

      xml_mask->study();
      }

   uint32_t result = xml_mask->split(vec, xml, 0, 0, 0);

   U_RETURN(result);
}

/*
RFC 2396 gives the following regular expression for parsing URLs:

^(([^:/?#]+):)?(//([^/?#]*))?([^?#]*)(\?([^#]*))?(#(.*))?
 12            3  4          5       6  7        8 9

scheme    = $2
authority = $4
path      = $5
query     = $7
fragment  = $9

query and fragment can be considered optional,
scheme and authority could also be optional in the presence of a base url.
*/

#define U_REGEX_URL "(^(([^:/?#]+):)?(//([^/?#]*))?([^?#]*)(\?([^#]*))?(#(.*))?)"

bool UPCRE::isValidURL(const UString& url)
{
   U_TRACE(0, "UPCRE::isValidURL(%.*S)", U_STRING_TO_TRACE(url))

   if (url_mask == 0)
      {
      U_NEW_ULIB_OBJECT(url_mask, UPCRE(U_STRING_FROM_CONSTANT(U_REGEX_URL), PCRE_FOR_REPLACE));

      url_mask->study();
      }

   bool result = url_mask->search(url, 0, 0, false);

   U_RETURN(result);
}

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UPCRE::dump(bool _reset) const
{
   *UObjectIO::os << "p_pcre                        " << (void*)p_pcre       << '\n'
                  << "_flags                        " << _flags              << '\n'
                  << "sub_len                       " << sub_len             << '\n'
                  << "sub_vec                       " << (void*)sub_vec      << '\n'
                  << "global_t                      " << global_t            << '\n'
                  << "replace_t                     " << replace_t           << '\n'
                  << "did_match                     " << did_match           << '\n'
                  << "stringlist                    " << (void*)stringlist   << '\n'
                  << "num_matches                   " << num_matches         << '\n'
                  << "p_pcre_extra                  " << (void*)p_pcre_extra << '\n'
                  << "_expression (UString          " << (void*)&_expression << ")\n"
                  << "resultset   (UVector<UString> " << (void*)resultset    << ')';

   if (_reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
