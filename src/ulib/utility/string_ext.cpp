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

      (void) U_SYSCALL(memcpy, "%p,%p,%u", result.data(), buffer, len);

      result.size_adjust(len);

      // only bio needs to be freed :)

      (void) U_SYSCALL(BIO_free, "%p", bio);

      U_RETURN_STRING(result);
      }

   U_RETURN_STRING(UString::getStringNull());
}
#endif

#ifdef HAVE_PCRE
// Searches subject for matches to pattern and replaces them with replacement

UString UStringExt::pregReplace(const UString& pattern, const UString& replacement, const UString& subject)
{
   U_TRACE(0, "UStringExt::pregReplace(%.*S,%.*S,%.*S)", U_STRING_TO_TRACE(pattern), U_STRING_TO_TRACE(replacement), U_STRING_TO_TRACE(subject))

   // Replace parts of a string using regular expressions. This method is the counterpart of the perl s// operator.
   // It replaces the substrings which matched the given regular expression (given to the constructor) with the supplied string

   UString lpattern = (isDelimited(pattern, "()") ?       pattern
                                                  : '(' + pattern + ')');

   UPCRE pcre(lpattern);

   UString result = pcre.replace(subject, replacement);

   U_RETURN_STRING(result);
}
#endif

UString UStringExt::expandTab(const char* s, uint32_t n, int tab)
{
   U_TRACE(1, "UStringExt::expandTab(%.*S,%u,%d)", n, s, n, tab)

   void* p;
   char* r;
   UString x(U_CAPACITY);
   uint32_t start = 0, end, num, len;

   while ((p = (void*)memchr(s + start, '\t', n - start)))
      {
      end = (const char*)p - s;

      if (end > start)
         {
         len = end - start;

         (void) x.reserve(x.size() + len + tab);

         if (len)
            {
            (void) U_SYSCALL(memcpy, "%p,%p,%u", x.rep->end(), s + start, len);

            x.rep->_length += len;
            }
         }

      num = tab - (x.rep->_length % tab);

      U_INTERNAL_DUMP("start = %u end = %u num = %u", start, end, num)

      r = x.rep->data();

      while (num--) r[x.rep->_length++] = ' ';

      start = end + 1;
      }

   len = n - start;

   if (len) (void) x.append(s + start, len);

   U_INTERNAL_ASSERT(x.invariant())

   U_RETURN_STRING(x);
}

UString UStringExt::substitute(const char* s, uint32_t n, const char* a, uint32_t n1, const char* b, uint32_t n2)
{
   U_TRACE(1, "UStringExt::substitute(%.*S,%u,%.*S,%u,%.*S,%u)", n, s, n, n1, a, n1, n2, b, n2)

   U_INTERNAL_ASSERT_MAJOR_MSG(n,0,"elaborazione su stringa vuota: inserire if empty()...")

   void* p;
   uint32_t start = 0, end, len;
   UString x(U_min((n / n1) * (n2 ? n2 : 1), 256 * 1024 * 1024)); // caso peggiore...

   while ((p = u_find(s + start, n - start, a, n1)))
      {
      end = (const char*)p - s;
      len = (end > start ? end - start : 0);

      U_INTERNAL_DUMP("start = %u end = %u len = %u", start, end, len)

      (void) x.reserve(x.size() + len + n2);

      if (len)
         {
         (void) U_SYSCALL(memcpy, "%p,%p,%u", x.rep->end(), s + start, len);

         x.rep->_length += len;
         }

      (void) U_SYSCALL(memcpy, "%p,%p,%u", x.rep->end(), b, n2);

      x.rep->_length += n2;

      start = end + n1;
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
   const char* end   = s.end();
         char* str   = result.data();
         char* start = str;

   while (ptr < end)
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

UString UStringExt::expandPath(const char* path_data, uint32_t path_size)
{
   U_TRACE(1, "UStringExt::expandPath(%.*S,%u)", path_size, path_data, path_size)

   UString pathname, path(U_CAPACITY);

   path.assign(path_data, path_size);

   path_data = path.data();

   if (path_data[0] == '~')
      {
      // expand ~/... and ~user/...

      struct passwd* pw;

      uint32_t len = path_size - 1;
      char* ptr    = (char*) memchr(++path_data, '/', len);

      if (ptr == 0) ptr = (char*)path_data + len;

      *ptr = '\0';

      if (ptr == path_data) pathname = U_SYSCALL(getenv, "%S", "HOME");
      else
         {
         pw = (struct passwd*) U_SYSCALL(getpwnam, "%S", path_data);

         if (pw &&
             pw->pw_dir)
            {
            pathname = pw->pw_dir;
            }
         }

      if (ptr != (path_data + len))
         {
         uint32_t tmp = u_strlen(path_data);

         *ptr = '/';

         pathname.append(ptr, len - tmp);
         }
      }
   else if (path_data[0] == '$')
      {
      // expand $var... and $var/...

      uint32_t len = path_size - 1;
      char* ptr    = (char*) memchr(++path_data, '/', len);

      if (ptr == 0) ptr = (char*)path_data + len;

      *ptr = '\0';

      char* envvar = U_SYSCALL(getenv, "%S", path_data);

      if (envvar) (void) pathname.assign(envvar);

      if (ptr != (path_data + len))
         {
         uint32_t tmp = u_strlen(path_data);

         *ptr = '/';

         (void) pathname.append(ptr, len - tmp);
         }
      }
   else
      {
      pathname = path;
      }

   U_INTERNAL_ASSERT(pathname.invariant())

   U_RETURN_STRING(pathname);
}

// recursively expand environment variables if needed

UString UStringExt::expandEnvVar(const char* s, uint32_t n)
{
   U_TRACE(1, "UStringExt::expandEnvVar(%.*S,%u)", n, s, n)

   U_INTERNAL_ASSERT_MAJOR_MSG(n,0,"elaborazione su stringa vuota: inserire if empty()...")

   char* ptr;
   char* value;
   UString x(n);
   const char* p;
   char name[128];
   uint32_t end, len, n1;

   while ((p = (const char*) memchr(s, '$', n)))
      {
      len = p - s;
      n  -= len;

      U_INTERNAL_DUMP("len = %u n = %u", len, n)

      // read name $var

      for (ptr = name, end = 1; (end < n && u_isspace(p[end]) == false && p[end] != '$'); ++end) *ptr++ = p[end];
                                                                                                 *ptr   = '\0';

      value = U_SYSCALL(getenv, "%S", name);

      n1 = (value ? strlen(value) : 0);

      // check for space

      if (n1 > end) (void) x.reserve(x.size() + (n1 - end));

      ptr = x.rep->end();

      if (n1)
         {
         (void) U_SYSCALL(memcpy, "%p,%p,%u", ptr,           s, len);
         (void) U_SYSCALL(memcpy, "%p,%p,%u", ptr + len, value,  n1);

         x.rep->_length += len + n1;
         }
      else
         {
         (void) U_SYSCALL(memcpy, "%p,%p,%u", ptr, s, len + end);

         x.rep->_length += len + end;
         }

      s  = p + end;
      n -= end;
      }

   if (n) (void) x.append(s, n);

   U_INTERNAL_ASSERT(x.invariant())

   U_RETURN_STRING(x);
}

// Returns a string that has the delimiter escaped

UString UStringExt::insertEscape(const char* s, uint32_t n, char delimiter)
{
   U_TRACE(1, "UStringExt::insertEscape(%.*S,%u,%C)", n, s, n, delimiter)

   U_INTERNAL_ASSERT_MAJOR_MSG(n,0,"elaborazione su stringa vuota: inserire if empty()...")

   U_INTERNAL_ASSERT_POINTER(memchr(s, delimiter, n))

   char* p;
   uint32_t sz, sz1 = 0;
   UString result(n * 2);
   const char* end = s + n;
   char* str = result.data();

   while (s < end)
      {
      p = (char*) memchr(s, delimiter, end - s);

      if (p)
         {
         sz = p - s;

         (void) U_SYSCALL(memcpy, "%p,%p,%u", str, s, sz);

         s    = p + 1;
         str += sz;

         *str++ = '\\';
         *str++ = delimiter;

         sz1 += sz + 2;
         }
      else
         {
         sz = end - s;

         (void) U_SYSCALL(memcpy, "%p,%p,%u", str, s, sz);

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
   U_TRACE(1, "UStringExt::removeEscape(%.*S,%u,%C)", n, s, n)

   U_INTERNAL_ASSERT_MAJOR_MSG(n,0,"elaborazione su stringa vuota: inserire if empty()...")

   U_INTERNAL_ASSERT_POINTER(memchr(s, '\\', n))

   char* p;
   UString result(n);
   uint32_t sz, sz1 = 0;
   const char* end = s + n;
   char* str = result.data();

   while (s < end)
      {
      p = (char*) memchr(s, '\\', end - s);

      if (p)
         {
         sz = p - s;

         (void) U_SYSCALL(memcpy, "%p,%p,%u", str, s, sz);

         s    = p + 1;
         str += sz;

         sz1 += sz;
         }
      else
         {
         sz = end - s;

         (void) U_SYSCALL(memcpy, "%p,%p,%u", str, s, sz);

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
   U_TRACE(1, "UStringExt::trim(%.*S,%u)", n, s, n)

// U_INTERNAL_ASSERT_MAJOR_MSG(n,0,"elaborazione su stringa vuota: inserire if empty()...")

   int start, end;
   UString result(n);

   // skip white space from start

   for (start = 0, end = n - 1; start <= end; ++start) // gcc - cannot optimize possibly infinite loops ???
      {
      if (u_isspace(s[start]) == false) break;
      }

   if (start <= end) // not only white space
      {
      while (end && u_isspace(s[end])) --end; // skip white space from end

      int sz = end - start + 1;

      if (sz)
         {
         (void) U_SYSCALL(memcpy, "%p,%p,%u", result.data(), &s[start], sz);

         result.size_adjust(sz);
         }
      }

   U_RETURN_STRING(result);
}

// returns a string that has whitespace removed from the start and the end, and which has each sequence of internal
// whitespace replaced with a single space.

UString UStringExt::simplifyWhiteSpace(const char* s, uint32_t n)
{
   U_TRACE(1, "UStringExt::simplifyWhiteSpace(%.*S,%u)", n, s, n)

   // U_INTERNAL_ASSERT_MAJOR_MSG(n,0,"elaborazione su stringa vuota: inserire if empty()...")

   UString result(n);
   uint32_t sz1, sz = 0;
   char* str = result.data();

   const char* p;
   const char* end = s + n;

   while (s < end)
      {
      // skip white space from start

      if (u_isspace(*s))
         {
         ++s;

         continue;
         }

      p = s++;

      while (s < end &&
             u_isspace(*s) == false)
         {
         ++s;
         }

      sz1 = (s - p);

      (void) U_SYSCALL(memcpy, "%p,%p,%u", str + sz, p, sz1); // result.append(p, sz1);

      sz += sz1;

      if (++s < end) str[sz++] = ' ';
      }

   if (sz > 0 && str[sz-1] == ' ') --sz;

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

   UString result = (pos == U_NOT_FOUND ? s : s.substr(pos+1));

   U_INTERNAL_ASSERT(result.invariant())

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

int UStringExt::compareversion(const char* a, uint32_t alen, const char* b, uint32_t blen)
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

      cval = U_SYSCALL(memcmp, "%p,%p,%u", a+apos1, b+bpos1, U_min(n1, n2));

      if (cval) U_RETURN(cval < 1 ? -1 : 1);
      }

   /* ran out of characters in one string, without finding a difference */

   /* maybe they were the same version, but with different leading zeros */
   if (apos2 == alen && bpos2 == blen) U_RETURN(0);

   /* the version with a suffix remaining is greater */
   U_RETURN(apos2 < alen ? 1 : -1);
}

UString UStringExt::compress(const UString& s)
{
   U_TRACE(1, "UStringExt::compress(%.*S)", U_STRING_TO_TRACE(s))

   uint32_t sz = s.size();
   UString r(U_CONSTANT_SIZE(U_LZOP_COMPRESS) + sizeof(uint32_t) + UCompress::space(sz));
   char* ptr = r.data();

   // copy magic byte

   (void) U_SYSCALL(memcpy, "%p,%p,%u", ptr, U_CONSTANT_TO_PARAM(U_LZOP_COMPRESS));

   ptr += U_CONSTANT_SIZE(U_LZOP_COMPRESS);

   // copy original size

   (void) U_SYSCALL(memcpy, "%p,%p,%u", ptr, &sz, sizeof(uint32_t));

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

#ifdef HAVE_LIBZ
   // decompress with zlib

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

      if (form)
         {
         // name is URL encoded...

         name.setBuffer(len);

         Url::decode(content.c_pointer(oldPos), len, name);

         name_value.push_back(name);
         }
      else
         {
         // name is already decoded...

         name_value.push_back(content.substr(oldPos, len));
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
   U_TRACE(1, "UStringExt::buildTokenInt(%S,%u,%.*S)", token, value, U_STRING_TO_TRACE(buffer))

   U_INTERNAL_ASSERT_POINTER(token)
   U_INTERNAL_ASSERT(strlen(token) == U_TOKEN_NM)

   uint32_t start = buffer.size();

   char* ptr = buffer.c_pointer(start);

   (void) U_SYSCALL(memcpy, "%p,%p,%u", ptr, token, U_TOKEN_NM);

   u_int2hex(ptr + U_TOKEN_NM, value);

   buffer.size_adjust(start + U_TOKEN_LN);
}
