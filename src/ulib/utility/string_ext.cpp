// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    string_ext.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/url.h>
#include <ulib/tokenizer.h>
#include <ulib/utility/compress.h>
#include <ulib/utility/string_ext.h>

#ifdef HAVE_PCRE
#  include <ulib/pcre/pcre.h>
#endif

#ifdef HAVE_LIBZ
#  include <ulib/base/coder/gzio.h>
#endif

#ifndef __MINGW32__
#  include <pwd.h>
#endif

#include <errno.h>

#ifdef HAVE_SSL
UString UStringExt::BIOtoString(BIO* bio)
{
   U_TRACE(1, "UStringExt::BIOtoString(%p)", bio)

   char* buffer = 0;
   long len     = BIO_get_mem_data(bio, &buffer);

   if (len > 0)
      {
      UString result(len);

      (void) u_memcpy(result.data(), buffer, len);

      result.size_adjust(len);

      // only bio needs to be freed :)

      (void) U_SYSCALL(BIO_free, "%p", bio);

      U_RETURN_STRING(result);
      }

   U_RETURN_STRING(UString::getStringNull());
}
#endif

#ifdef HAVE_PCRE
// Replace parts of a string using regular expressions. This method is the counterpart of the perl s// operator.
// It replaces the substrings which matched the given regular expression with the supplied string

UString UStringExt::pregReplace(const UString& pattern, const UString& replacement, const UString& subject)
{
   U_TRACE(0, "UStringExt::pregReplace(%.*S,%.*S,%.*S)", U_STRING_TO_TRACE(pattern), U_STRING_TO_TRACE(replacement), U_STRING_TO_TRACE(subject))

   UPCRE _pcre(pattern, PCRE_FOR_REPLACE);

   UString result = _pcre.replace(subject, replacement);

   U_RETURN_STRING(result);
}
#endif

UString UStringExt::numberToString(off_t n, bool abbrev)
{
   U_TRACE(0, "UStringExt::numberToString(%I,%b)", n, abbrev)

   UString x(32U);

   if (abbrev == false) x.snprintf("%I", n);
   else
      {
      u_printSize(x.data(), n);

      x.size_adjust();
      }

   U_RETURN_STRING(x);
}

UString UStringExt::expandTab(const char* s, uint32_t n, int tab)
{
   U_TRACE(1, "UStringExt::expandTab(%.*S,%u,%d)", n, s, n, tab)

   void* p;
   char* r;
   UString x(U_CAPACITY);
   uint32_t start = 0, _end, num, len;

   while ((p = (void*)memchr(s + start, '\t', n - start)))
      {
      _end = (const char*)p - s;

      if (_end > start)
         {
         len = _end - start;

         (void) x.reserve(x.size() + len + tab);

         if (len)
            {
            (void) u_memcpy(x.rep->end(), s + start, len);

            x.rep->_length += len;
            }
         }

      num = tab - (x.rep->_length % tab);

      U_INTERNAL_DUMP("start = %u _end = %u num = %u", start, _end, num)

      r = x.rep->data();

      while (num--) r[x.rep->_length++] = ' ';

      start = _end + 1;
      }

   len = n - start;

   if (len) (void) x.append(s + start, len);

   U_INTERNAL_ASSERT(x.invariant())

   U_RETURN_STRING(x);
}

UString UStringExt::substitute(const char* s, uint32_t n, const char* a, uint32_t n1, const char* b, uint32_t n2)
{
   U_TRACE(1, "UStringExt::substitute(%.*S,%u,%.*S,%u,%.*S,%u)", n, s, n, n1, a, n1, n2, b, n2)

   U_INTERNAL_ASSERT_MAJOR_MSG(n, 0, "elaborazione su stringa vuota: inserire if empty()...")

   void* p;
   uint32_t start = 0, _end, len;
   UString x(U_min((n / n1) * (n2 ? n2 : 1), 256 * 1024 * 1024)); // caso peggiore...

   while ((p = u_find(s + start, n - start, a, n1)))
      {
      _end = (const char*)p - s;
      len  = (_end > start ? _end - start : 0);

      U_INTERNAL_DUMP("start = %u _end = %u len = %u", start, _end, len)

      (void) x.reserve(x.size() + len + n2);

      if (len)
         {
         (void) u_memcpy(x.rep->end(), s + start, len);

         x.rep->_length += len;
         }

      (void) u_memcpy(x.rep->end(), b, n2);

      x.rep->_length += n2;

      start = _end + n1;
      }

   len = n - start;

   if (len) (void) x.append(s + start, len);

   U_INTERNAL_ASSERT(x.invariant())

   U_RETURN_STRING(x);
}

// dos2unix '\n' convertor

UString UStringExt::dos2unix(const UString& s, bool unix2dos)
{
   U_TRACE(0, "UStringExt::dos2unix(%.*S,%b)", U_STRING_TO_TRACE(s), unix2dos)

   UString result(s.size() * 2);

   char c;
   const char* ptr   = s.data();
   const char* _end  = s.end();
         char* str   = result.data();
         char* start = str;

   while (ptr < _end)
      {
      c = *ptr++;

      if (c == '\r') continue;
      if (c == '\n')
         {
         if (unix2dos) *str++ = '\r';
                       *str++ = '\n';

         continue;
         }

      *str++ = c;
      }

   result.size_adjust(str - start);

   U_RETURN_STRING(result);
}

UString UStringExt::expandPath(const char* s, uint32_t n, const UString* environment)
{
   U_TRACE(0, "UStringExt::expandPath(%.*S,%u,%p)", n, s, n, environment)

   U_INTERNAL_ASSERT_MAJOR_MSG(n, 0, "elaborazione su stringa vuota: inserire if empty()...")

   char c = *s;
   UString x(n+100);

   if (c == '~' ||
       c == '$')
      {
      UString value;
      uint32_t _end = 1;

      while (_end < n && s[_end] != '/') ++_end;

      U_INTERNAL_DUMP("_end = %u", _end)

      if (_end == 1)
         {
         if (c == '$') goto end;

         // expand ~/...

         value = getEnvironmentVar(U_CONSTANT_TO_PARAM("HOME"), environment);
         }
      else if (c == '$')
         {
         // expand $var... and $var/...

         value = getEnvironmentVar(s + 1, _end - 1, environment);
         }
      else
         {
         // expand ~user/...

         char buffer[128];

         U_INTERNAL_ASSERT_MINOR(_end, sizeof(buffer))

         (void) u_memcpy(buffer, s + 1, _end - 1);

         buffer[_end-1] = '\0';

         struct passwd* pw = (struct passwd*) U_SYSCALL(getpwnam, "%S", buffer);

         if (pw && pw->pw_dir) (void) value.assign(pw->pw_dir);
         }

      s += _end;
      n -= _end;

      (void) x.append(value);
      }

end:
   if (n) (void) x.append(s, n);

   U_INTERNAL_ASSERT(x.invariant())

   U_RETURN_STRING(x);
}

// recursively expand environment variables if needed

UString UStringExt::expandEnvironmentVar(const char* s, uint32_t n, const UString* environment)
{
   U_TRACE(0, "UStringExt::expandEnvironmentVar(%.*S,%u,%p)", n, s, n, environment)

   U_INTERNAL_ASSERT_MAJOR_MSG(n, 0, "elaborazione su stringa vuota: inserire if empty()...")

   char* p2;
   const char* p;
   const char* p1 = 0;
   UString value, x(n+100);
   uint32_t _end, len, n1, n2 = 0;

   while ((p = (const char*) memchr(s, '$', n)))
      {
      len = p - s;
      n  -= len;

      // read name $var

      _end = 1;

      while (_end < n && (u_isalnum(p[_end]) || p[_end] == '_'))
         {
         U_INTERNAL_ASSERT_DIFFERS(p[_end], '$')
         U_INTERNAL_ASSERT_EQUALS(u_isspace(p[_end]), false)

         ++_end;
         }

      U_INTERNAL_DUMP("len = %u n = %u _end = %u", len, n, _end)

      if (_end == 1) n1 = 0;
      else
         {
         value = getEnvironmentVar(p + 1, _end - 1, environment);

         if (n2 &&
             value.find('$', 0U) != U_NOT_FOUND)
            {
            value = getEnvironmentVar(p + 1, _end - 1, &x);
            }

         n1 = value.size();
         p1 = value.data();
         }

      // check for space

      if (n1 > _end) (void) x.reserve(n2 + (n1 - _end));

      p2 = x.c_pointer(n2);

      if (n1)
         {
         if (len) (void) u_memcpy(p2,        s, len);
                  (void) u_memcpy(p2 + len, p1,  n1);

         n2 += n1;
         }
      else
         {
         if (len) (void) u_memcpy(p2, s, len);
         }

      n2 += len;

      x.size_adjust(n2);

      s  = p + _end;
      n -=     _end;
      }

   if (n) (void) x.append(s, n);

   U_INTERNAL_ASSERT(x.invariant())

   U_RETURN_STRING(x);
}

UString UStringExt::getEnvironmentVar(const char* s, uint32_t n, const UString* environment)
{
   U_TRACE(1, "UStringExt::getEnvironmentVar(%.*S,%u,%p)", n, s, n, environment)

   UString value(100U);

   if (environment)
      {
      char c, c1;
      const char* end;
      uint32_t start = 0;
      bool quoted, bexpand;

loop: // NB: check if s param is a environment-var
      start = environment->find(s, start, n);

      if (start == U_NOT_FOUND) goto next;

      c = '\0';

      if (start)
         {
         // NB: check if comment...

         c = environment->c_char(start-1);

         U_INTERNAL_DUMP("c = %C", c)

         if (c == '#') goto loop;
         }

      start += n;

      c1 = environment->c_char(start);

      U_INTERNAL_DUMP("c1 = %C", c1)

      if (c1 != '=') goto loop;

      quoted  = (c == '\'' || c == '"');
      bexpand = false;

      U_INTERNAL_DUMP("quoted = %b", quoted)

      s   = environment->c_pointer(++start);
      end = environment->end();

      U_INTERNAL_DUMP("end - s = %ld", end - s)

      if (s < end)
         {
         const char* ptr = s;

         do {
            if ((c1 = *ptr) == '$') bexpand = true;

            if (quoted)
               {
               if (c1 != c || ptr[-1] == '\\') continue;
               }
            else
               {
               if (u_isspace(c1) == false) continue;
               }

            U_INTERNAL_DUMP("ptr - s = %ld", ptr - s)

            if (ptr == s) goto end; // NB: name=<empty>...

            n = ptr - s;

            goto assign;
            }
         while (++ptr < end);

         n = end - s;
assign:
         U_INTERNAL_DUMP("n = %u", n)

         U_INTERNAL_ASSERT_MAJOR(n, 0)

         if (bexpand) value = expandEnvironmentVar(s, n, environment);
         else  (void) value.assign(s, n);
         }
      }
   else
      {
next:
      char buffer[128];

      U_INTERNAL_ASSERT_MINOR(n, sizeof(buffer))

      (void) u_memcpy(buffer, s,  n);

      buffer[n] = '\0';

      const char* ptr = U_SYSCALL(getenv, "%S", buffer);

      if (ptr) (void) value.assign(ptr);
      }
end:
   U_RETURN_STRING(value);
}

UString UStringExt::getPidProcess()
{
   U_TRACE(0, "UStringExt::getPidProcess()")

   UString value(32U);

   (void) u_memcpy(value.data(), u_pid_str, u_pid_str_len);

   value.size_adjust(u_pid_str_len);

   U_RETURN_STRING(value);
}

extern void* expressionParserAlloc(void* (*mallocProc)(size_t));
extern void  expressionParserFree(void* p, void (*freeProc)(void*));
extern void  expressionParserTrace(FILE* stream, char* zPrefix);
extern void  expressionParser(void* yyp, int yymajor, UString* yyminor, UString* result);

UString UStringExt::evalExpression(const UString& expr, const UString& environment)
{
   U_TRACE(0, "UStringExt::evalExpression(%.*S,%.*S)", U_STRING_TO_TRACE(expr), U_STRING_TO_TRACE(environment))

   int token_id;
   UTokenizer t(expr);
   UString token, result(U_CONSTANT_TO_PARAM("true"));

   void* pParser = expressionParserAlloc(malloc);

#ifdef DEBUG
   (void) fprintf(stderr, "start parsing expr: \"%.*s\"\n", U_STRING_TO_TRACE(expr));

   expressionParserTrace(stderr, (char*)"parser: ");
#endif

   while (result.empty() == false &&
          (token_id = t.getTokenId(token)) > 0)
      {
      if (token_id == U_TK_NAME)
         {
         token    = UStringExt::getEnvironmentVar(token, &environment);
         token_id = U_TK_VALUE;
         }
      else if (token_id == U_TK_PID)
         {
         token    = UStringExt::getPidProcess();
         token_id = U_TK_VALUE;
         }

      expressionParser( pParser, token_id, U_NEW(UString(token)), &result);
      }

   expressionParser(    pParser,        0,                     0, &result);

   expressionParserFree(pParser, free);

#ifdef DEBUG
   (void) fprintf(stderr, "ended parsing expr: \"%.*s\"\n", U_STRING_TO_TRACE(expr));
#endif

   U_RETURN_STRING(result);
}

// Returns a string that has the delimiter escaped

UString UStringExt::insertEscape(const char* s, uint32_t n, char delimiter)
{
   U_TRACE(0, "UStringExt::insertEscape(%.*S,%u,%C)", n, s, n, delimiter)

   U_INTERNAL_ASSERT_MAJOR_MSG(n,0,"elaborazione su stringa vuota: inserire if empty()...")

   U_INTERNAL_ASSERT_POINTER(memchr(s, delimiter, n))

   char* p;
   uint32_t sz, sz1 = 0;
   UString result(n * 2);
   const char* _end = s + n;
   char* str = result.data();

   while (s < _end)
      {
      p = (char*) memchr(s, delimiter, _end - s);

      if (p)
         {
         sz = p - s;

         (void) u_memcpy(str, s, sz);

         s    = p + 1;
         str += sz;

         *str++ = '\\';
         *str++ = delimiter;

         sz1 += sz + 2;
         }
      else
         {
         sz = _end - s;

         (void) u_memcpy(str, s, sz);

         sz1 += sz;

         break;
         }
      }

   result.size_adjust(sz1);

   U_RETURN_STRING(result);
}

// manage escaping for delimiter character

UString UStringExt::removeEscape(const char* s, uint32_t n)
{
   U_TRACE(0, "UStringExt::removeEscape(%.*S,%u,%C)", n, s, n)

   U_INTERNAL_ASSERT_MAJOR_MSG(n,0,"elaborazione su stringa vuota: inserire if empty()...")

   U_INTERNAL_ASSERT_POINTER(memchr(s, '\\', n))

   char* p;
   UString result(n);
   uint32_t sz, sz1 = 0;
   const char* _end = s + n;
   char* str = result.data();

   while (s < _end)
      {
      p = (char*) memchr(s, '\\', _end - s);

      if (p)
         {
         sz = p - s;

         (void) u_memcpy(str, s, sz);

         s    = p + 1;
         str += sz;

         sz1 += sz;
         }
      else
         {
         sz = _end - s;

         (void) u_memcpy(str, s, sz);

         sz1 += sz;

         break;
         }
      }

   result.size_adjust(sz1);

   U_RETURN_STRING(result);
}

// Returns a string that has whitespace removed from the start and the end.

UString UStringExt::trim(const char* s, uint32_t n)
{
   U_TRACE(0, "UStringExt::trim(%.*S,%u)", n, s, n)

   // U_INTERNAL_ASSERT_MAJOR_MSG(n,0,"elaborazione su stringa vuota: inserire if empty()...")

   int32_t i = 0;
   UString result(n);

   // skip white space from start

   while (i < (int32_t)n && u_isspace(s[i])) ++i;

   U_INTERNAL_DUMP("i = %d", i)

   if (i < (int32_t)n) // not only white space
      {
      while (u_isspace(s[--n])) {} // skip white space from end

      n += 1 - i;

      (void) u_memcpy(result.data(), s+i, n);

      result.size_adjust(n);
      }

   U_RETURN_STRING(result);
}

// Returns a string that has any printable character which is not a space or
// an alphanumeric character removed from the start and the end (leading and trailing)

UString UStringExt::trimPunctuation(const char* s, uint32_t n)
{
   U_TRACE(0, "UStringExt::trimPunctuation(%.*S,%u)", n, s, n)

   // U_INTERNAL_ASSERT_MAJOR_MSG(n,0,"elaborazione su stringa vuota: inserire if empty()...")

   int32_t i = 0;
   UString result(n);

   // skip punctuation character from start

   while (i < (int32_t)n && u_ispunct(s[i])) ++i;

   U_INTERNAL_DUMP("i = %d", i)

   if (i < (int32_t)n) // not only punctuation character
      {
      while (u_ispunct(s[--n])) {} // skip punctuation character from end

      n += 1 - i;

      (void) u_memcpy(result.data(), s+i, n);

      result.size_adjust(n);
      }

   U_RETURN_STRING(result);
}

// returns a string that has whitespace removed from the start and the end, and which has each sequence of internal
// whitespace replaced with a single space.

UString UStringExt::simplifyWhiteSpace(const char* s, uint32_t n)
{
   U_TRACE(0, "UStringExt::simplifyWhiteSpace(%.*S,%u)", n, s, n)

// U_INTERNAL_ASSERT_MAJOR_MSG(n,0,"elaborazione su stringa vuota: inserire if empty()...")

   UString result(n);
   uint32_t sz1, sz = 0;
   char* str = result.data();

   const char* p;
   const char* _end = s + n;

   while (s < _end)
      {
      if (u_isspace(*s))
         {
         ++s;

         continue;
         }

      p = s++;

      while (s < _end &&
             u_isspace(*s) == false)
         {
         ++s;
         }

      sz1 = (s - p);

      (void) u_memcpy(str + sz, p, sz1); // result.append(p, sz1);

      sz += sz1;

      if (++s < _end) str[sz++] = ' ';
      }

   if (sz && u_isspace(str[sz-1])) --sz;

   result.size_adjust(sz);

   U_RETURN_STRING(result);
}

// returns a string that has suppressed repeated empty lines

UString UStringExt::removeEmptyLine(const char* s, uint32_t n)
{
   U_TRACE(0, "UStringExt::removeEmptyLine(%.*S,%u)", n, s, n)

// U_INTERNAL_ASSERT_MAJOR_MSG(n,0,"elaborazione su stringa vuota: inserire if empty()...")

   UString result(n);
   uint32_t sz1, sz = 0;
   char* str = result.data();

   const char* p;
   const char* _end = s + n;

   while (s < _end)
      {
      if (u_islterm(*s))
         {
         ++s;

         continue;
         }

      p = s++;

      while (s < _end &&
             u_islterm(*s) == false)
         {
         ++s;
         }

      sz1 = (s - p);

      (void) u_memcpy(str + sz, p, sz1); // result.append(p, sz1);

      sz += sz1;

      if (++s < _end) str[sz++] = '\n';
      }

   if (sz && u_islterm(str[sz-1])) --sz;

   result.size_adjust(sz);

   U_RETURN_STRING(result);
}

UString UStringExt::dirname(const UString& s)
{
   U_TRACE(0, "UStringExt::dirname(%.*S)", U_STRING_TO_TRACE(s))

   UString result;
   uint32_t pos = s.rfind('/'); /* Find last '/' */

   char* path        = s.data();
   char* runp        = s.c_pointer(pos);
   char* last_slash  = (pos == U_NOT_FOUND ? 0 : path + pos);

   if (last_slash         &&
       last_slash != path &&
       ((pos + 1) == s.size()))
      {
      // Determine whether all remaining characters are slashes

      for (; runp != path; --runp) if (runp[-1] != '/') break;

      // The '/' is the last character, we have to look further

      if (runp != path) last_slash = (char*) memrchr(path, '/', runp - path);
      }

   if (last_slash == 0)
      {
      // This assignment is ill-designed but the XPG specs require to
      // return a string containing "." in any case no directory part is
      // found and so a static and constant string is required

      result = U_STRING_FROM_CONSTANT(".");
      }
   else
      {
      // Determine whether all remaining characters are slashes

      for (runp = last_slash; runp != path; --runp) if (runp[-1] != '/') break;

      // Terminate the path

      if (runp == path)
         {
         // The last slash is the first character in the string.  We have to
         // return "/".  As a special case we have to return "//" if there
         // are exactly two slashes at the beginning of the string.  See
         // XBD 4.10 Path Name Resolution for more information

         if (last_slash == path + 1) ++last_slash;
         else                          last_slash = path + 1;
         }
      else
         {
         last_slash = runp;
         }

      result = s.substr(0U, (uint32_t)(last_slash - path));
      }

   U_INTERNAL_ASSERT(result.invariant())

   U_RETURN_STRING(result);
}

UString UStringExt::basename(const UString& s)
{
   U_TRACE(0, "UStringExt::basename(%.*S)", U_STRING_TO_TRACE(s))

   uint32_t pos = s.rfind('/'); /* Find last '/' */

   UString result = s.substr(pos+1);

   U_RETURN_STRING(result);
}

UString UStringExt::suffix(const UString& s, char sep)
{
   U_TRACE(0, "UStringExt::suffix(%.*S,%C)", U_STRING_TO_TRACE(s), sep)

   uint32_t pos = s.find_last_of(sep);

   if (pos == U_NOT_FOUND) U_RETURN_STRING(UString::getStringNull());

   UString result = s.substr(pos+1);

   U_INTERNAL_ASSERT(result.invariant())

   U_RETURN_STRING(result);
}

/* Sort two version numbers, comparing equivalently seperated strings of digits numerically.
 *
 * Returns a positive number if (a > b)
 * Returns a negative number if (a < b)
 * Returns zero if (a == b)
 */

__pure int UStringExt::compareversion(const char* a, uint32_t alen, const char* b, uint32_t blen)
{
   U_TRACE(0, "UStringExt::compareversion(%.*S,%u,%.*S,%u)", alen, a, alen, blen, b, blen)

   if (a == b) U_RETURN(0);

   int cval;
   bool isnum;
   uint32_t apos1, apos2 = 0, bpos1, bpos2 = 0;

   while (apos2 < alen &&
          bpos2 < blen)
      {
      apos1 = apos2;
      bpos1 = bpos2;

      if (u_isdigit(a[apos2]))
         {
         isnum = true;

         while (apos2 < alen && u_isdigit(a[apos2])) apos2++;
         while (bpos2 < blen && u_isdigit(b[bpos2])) bpos2++;
         }
      else
         {
         isnum = false;

         while (apos2 < alen && !u_isdigit(a[apos2])) apos2++;
         while (bpos2 < blen && !u_isdigit(b[bpos2])) bpos2++;
         }

      U_INTERNAL_ASSERT_DIFFERS(apos1,apos2) 

      /* isdigit(a[0]) != isdigit(b[0])
       * arbitrarily sort the non-digit first */

      if (bpos1 == bpos2) U_RETURN(isnum ? 1 : -1);

      if (isnum)
         {
         /* skip numeric leading zeros */
         while (apos1 < alen && a[apos1] == '0') apos1++;
         while (bpos1 < blen && b[bpos1] == '0') bpos1++;

         /* if one number has more digits, it is greater */
         if (apos2-apos1 > bpos2-bpos1) U_RETURN(1);
         if (apos2-apos1 < bpos2-bpos1) U_RETURN(-1);
         }

      /* do an ordinary lexicographic string comparison */

      uint32_t n1 = apos2-apos1,
               n2 = bpos2-bpos1;

      cval = memcmp(a+apos1, b+bpos1, U_min(n1, n2));

      if (cval) U_RETURN(cval < 1 ? -1 : 1);
      }

   /* ran out of characters in one string, without finding a difference */

   /* maybe they were the same version, but with different leading zeros */
   if (apos2 == alen && bpos2 == blen) U_RETURN(0);

   /* the version with a suffix remaining is greater */
   U_RETURN(apos2 < alen ? 1 : -1);
}

__pure int UStringExt::compareversion(const UString& s, const UString& a) { return compareversion(U_STRING_TO_PARAM(s), U_STRING_TO_PARAM(a)); }

__pure bool UStringExt::isEmailAddress(const UString& s)
{
   U_TRACE(0, "UStringExt::isEmailAddress(%.*S)", U_STRING_TO_TRACE(s))

   if (u_validate_email_address(U_STRING_TO_PARAM(s))) U_RETURN(true);

   U_RETURN(false);
}

UString UStringExt::compress(const UString& s)
{
   U_TRACE(1, "UStringExt::compress(%.*S)", U_STRING_TO_TRACE(s))

   uint32_t sz = s.size();
   UString r(U_CONSTANT_SIZE(U_LZOP_COMPRESS) + sizeof(uint32_t) + UCompress::space(sz));
   char* ptr = r.data();

   // copy magic byte

   (void) U_MEMCPY(ptr, U_LZOP_COMPRESS);

   ptr += U_CONSTANT_SIZE(U_LZOP_COMPRESS);

   // copy original size

   (void) u_memcpy(ptr, &sz, sizeof(uint32_t));

   ptr += sizeof(uint32_t);

   // compress with lzo

   r.rep->_length = U_CONSTANT_SIZE(U_LZOP_COMPRESS) + sizeof(uint32_t) + UCompress::compress(s.rep->str, sz, ptr);

   U_INTERNAL_ASSERT(r.invariant())
   U_INTERNAL_ASSERT(UStringExt::isCompress(r))

   U_RETURN_STRING(r);
}

UString UStringExt::decompress(const UString& s)
{
   U_TRACE(0, "UStringExt::decompress(%.*S)", U_STRING_TO_TRACE(s))

   // check magic byte

   U_INTERNAL_ASSERT(UStringExt::isCompress(s))

   // read original size

   char* ptr   = s.rep->data() + U_CONSTANT_SIZE(U_LZOP_COMPRESS);
   uint32_t sz = *((uint32_t*)ptr);

   UString r(sz + 32);

   ptr += sizeof(uint32_t);

   // decompress with lzo

   r.rep->_length = UCompress::decompress(ptr, s.size() - U_CONSTANT_SIZE(U_LZOP_COMPRESS) - sizeof(uint32_t), r.rep->data());

   U_INTERNAL_ASSERT(r.invariant())

   U_RETURN_STRING(r);
}

UString UStringExt::deflate(const UString& s) // .gz compress
{
   U_TRACE(0, "UStringExt::deflate(%.*S)", U_STRING_TO_TRACE(s))

   // compress with zlib

#ifdef HAVE_LIBZ
   UString r(s.rep->_length * 2);

   r.rep->_length = u_gz_deflate(s.rep->str, s.rep->_length, r.rep->data());

   U_INTERNAL_DUMP("u_gz_deflate() = %d", r.rep->_length)

   U_INTERNAL_ASSERT(r.invariant())

   U_RETURN_STRING(r);
#else
   U_RETURN_STRING(s);
#endif
}

UString UStringExt::gunzip(const UString& s, uint32_t sz) // .gz uncompress
{
   U_TRACE(0, "UStringExt::gunzip(%.*S,%u)", U_STRING_TO_TRACE(s), sz)

   if (sz == 0)
      {
      // check magic byte

      if (U_MEMCMP(s.rep->str, GZIP_MAGIC)) sz = s.rep->_length * 10;
      else
         {
         // read original size

         char* ptr = s.rep->data() + s.rep->_length - 4;
         sz        = *((uint32_t*)ptr);
         }
      }

   UString r(sz);

#ifdef HAVE_LIBZ // decompress with zlib
   r.rep->_length = u_gz_inflate(s.rep->str, s.rep->_length, r.rep->data());

   U_INTERNAL_DUMP("u_gz_inflate() = %d", r.rep->_length)
#endif

   U_INTERNAL_ASSERT(r.invariant())

   U_RETURN_STRING(r);
}

// convert letter to upper or lower case

UString UStringExt::tolower(const UString& x)
{
   U_TRACE(0, "UStringExt::tolower(%.*S)", U_STRING_TO_TRACE(x))

   U_INTERNAL_ASSERT_MAJOR_MSG(x.rep->_length,0,"elaborazione su stringa vuota: inserire if empty()...")

   UString r(x.rep->_length);

   r.rep->_length = x.rep->_length;

   char* ptr = r.rep->data();

   const char* s   = x.rep->str;
   const char* end = s + x.rep->_length;

   while (s < end) *ptr++ = u_tolower(*s++);

   U_INTERNAL_ASSERT(r.invariant())

   U_RETURN_STRING(r);
}

UString UStringExt::toupper(const UString& x)
{
   U_TRACE(0, "UStringExt::toupper(%.*S)", U_STRING_TO_TRACE(x))

   U_INTERNAL_ASSERT_MAJOR_MSG(x.rep->_length,0,"elaborazione su stringa vuota: inserire if empty()...")

   UString r(x.rep->_length);

   r.rep->_length = x.rep->_length;

   char* ptr = r.rep->data();

   const char* s   = x.rep->str;
   const char* end = s + x.rep->_length;

   while (s < end) *ptr++ = u_toupper(*s++);

   U_INTERNAL_ASSERT(r.invariant())

   U_RETURN_STRING(r);
}

// retrieve information on form elements as couple <name1>=<value1>&<name2>=<value2>&...

uint32_t UStringExt::getNameValueFromData(const UString& content, UVector<UString>& name_value, const char* delim, uint32_t dlen)
{
   U_TRACE(0, "UStringExt::getNameValueFromData(%.*S,%p,%.*S,%u)", U_STRING_TO_TRACE(content), &name_value, dlen, delim, dlen)

   U_INTERNAL_ASSERT_POINTER(delim)
   U_ASSERT_EQUALS(content.empty(),false)

   // Parse the data in one fell swoop for efficiency

   const char* ptr;
   const char* end = content.end();

   UString name(U_CAPACITY), value(U_CAPACITY);

   bool form  = (dlen == 1 && *delim == '&');
   uint32_t n = name_value.size(), size = content.size(), result, pos, oldPos = 0, len;

   while (true)
      {
      // Find the '=' separating the name from its value

      pos = content.find_first_of('=', oldPos);

      // If no '=', we're finished

      if (pos == U_NOT_FOUND) break;

      len = pos - oldPos;

      U_INTERNAL_ASSERT_MAJOR(len,0)

      if (form == false) name_value.push_back(content.substr(oldPos, len));
      else
         {
         // name is URL encoded...

         name.setBuffer(len);

         Url::decode(content.c_pointer(oldPos), len, name);

         name_value.push_back(name);
         }

      // Find the delimitator separating subsequent name/value pairs

      oldPos = ++pos;

      if (form) pos = content.find_first_of('&', oldPos);
      else
         {
         // check if string is quoted...

         if (content.c_char(pos) == '"')
            {
            ptr = u_find_char(content.c_pointer(++pos), end, '"');

            pos = content.distance(ptr);
            }

         pos = content.find_first_of(delim, pos, dlen);
         }

      // Even if an delimitator wasn't found the rest of the string is a value and value is already decoded...

      len = (pos == U_NOT_FOUND ? size : pos) - oldPos;

      if (len == 0) name_value.push_back(UString::getStringNull());
      else
         {
         if (form)
            {
            value.setBuffer(len);

            Url::decode(content.c_pointer(oldPos), len, value);
            }
         else
            {
            value = content.substr(oldPos, len);

            if (value.isQuoted()) value.unQuote();
            }

         name_value.push_back(value);
         }

      if (pos == U_NOT_FOUND) break;

      // Update parse position

      if (form) oldPos = ++pos;
      else
         {
         ptr = content.c_pointer(pos);

         do {
            if (++ptr >= end) goto end;
            }
         while (memchr(delim, *ptr, dlen));

         oldPos = content.distance(ptr);
         }
      }

end:
   result = (name_value.size() - n);

   U_RETURN(result);
}

void UStringExt::buildTokenInt(const char* token, uint32_t value, UString& buffer)
{
   U_TRACE(0, "UStringExt::buildTokenInt(%S,%u,%.*S)", token, value, U_STRING_TO_TRACE(buffer))

   U_INTERNAL_ASSERT_POINTER(token)
   U_INTERNAL_ASSERT(u_str_len(token) == U_TOKEN_NM)

   uint32_t start = buffer.size();

   char* ptr = buffer.c_pointer(start);

   (void) u_memcpy(ptr, token, U_TOKEN_NM);

   u_int2hex(ptr + U_TOKEN_NM, value);

   buffer.size_adjust(start + U_TOKEN_LN);
}

// Minifies CSS/JS by removing comments and whitespaces

static inline bool unextendable(char c)
{
   U_TRACE(0, "::unextendable(%C)", c)

   // return true for any character that never needs to be separated from other characters via whitespace

   switch (c)
      {
      case '[':
      case ']':
      case '{':
      case '}':
      case '/':
      case ';':
      case ':': U_RETURN(true);
      default:  U_RETURN(false);
      }
}

// return true for any character that must separated from other "extendable"
// characters by whitespace on the _right_ in order keep tokens separate.

static inline bool isExtendableOnRight(char c)
{
   U_TRACE(0, "::isExtendableOnRight(%C)", c)

   // NB: left paren only here -- see http://code.google.com/p/page-speed/issues/detail?id=339

   bool result = ((unextendable(c) || c == '(') == false);

   U_RETURN(result);
}

// return true for any character that must separated from other "extendable"
// characters by whitespace on the _left_ in order keep tokens separate.

static inline bool isExtendableOnLeft(char c)
{
   U_TRACE(0, "::isExtendableOnLeft(%C)", c)

   // NB: right paren only here

   bool result = ((unextendable(c) || c == ')') == false);

   U_RETURN(result);
}

void UStringExt::minifyCssJs(UString& x)
{
   U_TRACE(0, "UStringExt::minifyCssJs(%.*S)", U_STRING_TO_TRACE(x))

   uint32_t n = x.size();

// U_INTERNAL_ASSERT_MAJOR_MSG(n, 0, "elaborazione su stringa vuota: inserire if empty()...")

   char quote;
   const char* s;
   const char* start;
   const char* begin = x.data();
   uint32_t capacity, sz1, sz = 0;
   const char* _end  = (s = begin) + n;
   char* str = (char*) U_MALLOC_STR(n + 128, capacity);

   // we have these tokens: comment, whitespace, single/double-quoted string, and other

   while (s < _end)
      {
      if ( *s      == '/' &&
          *(s + 1) == '*' &&
           (s + 1) < _end)
         {
         // comment: scan to end of comment

         for (s += 2; s < _end; ++s)
            {
            if (*s      == '*' &&
               *(s + 1) == '/' &&
                (s + 1) < _end)
               {
               s += 2;

               break;
               }
            }
         }
      else if (*s       == '/' &&
               *(s + 1) == '/' &&
                (s + 1) < _end)
         {
         // comment: scan to end of comment

         for (s += 2; s < _end && *s != '\n'; ++s) {}
         }
      else if (u_isspace(*s))
         {
         // whitespace: scan to end of whitespace; put a single space into the
         // consumer if necessary to separate tokens, otherwise put nothing

         start = s;

         do { ++s; } while (s < _end && u_isspace(*s));

         if (s < _end                          &&
             start > begin                     &&
             isExtendableOnRight(*(start - 1)) &&
             isExtendableOnLeft(*s))
            {
            str[sz++] = ' ';
            }
         }
      else if (*s == '\'' ||
               *s == '"')
         {
         // single/double-quoted string: scan to end of string (first unescaped quote of the
         // same kind used to open the string), and put the whole string into the consumer

         start =  s;
         quote = *s++;

         while (s < _end)
            {
            if (*s == quote)
               {
               ++s;

               break;
               }
            else if (*s == '\\' && (s + 1) < _end)
               {
               s += 2;
               }
            else
               {
               ++s;
               }
            }

         sz1 = (s - start);

         (void) u_memcpy(str + sz, start, sz1);  // result.append(start, sz1);

         sz += sz1;
         }
      else
         {
         // other: just copy the character over

         str[sz++] = *s;

         if (*s == '}')
            {
            // add a newline after each closing brace to prevent output lines from being too long

            str[sz++] = '\n';
            }

         ++s;
         }

      if (sz >= n) goto end;
      }

   U_INTERNAL_ASSERT(sz <= n)

   (void) x.replace(str, sz);

end:
   U_FREE_STR(str, capacity);
}

// inlining failed in call to ...: call is unlikely and code size would grow

__pure bool UStringExt::startsWith(const UString& s1, const UString& s2) { return u_startsWith(U_STRING_TO_PARAM(s1), U_STRING_TO_PARAM(s2)); }
__pure bool UStringExt::endsWith(  const UString& s1, const UString& s2) { return u_endsWith(  U_STRING_TO_PARAM(s1), U_STRING_TO_PARAM(s2)); }
