/** ============================================================================
//
// = LIBRARY
//    ulibase - c library
//
// = FILENAME
//    utility.c
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================ */

/*
#define DEBUG_DEBUG
*/

#include <ulib/base/utility.h>

#include <ctype.h>
#include <stdlib.h>

#ifdef HAVE_SYSEXITS_H
#  include <sysexits.h>
#else
#  include <ulib/base/replace/sysexits.h>
#endif

#if HAVE_DIRENT_H
#  include <dirent.h>
#  ifdef _DIRENT_HAVE_D_NAMLEN
#     define NAMLEN(dirent) (dirent)->d_namlen
#  else
#     define NAMLEN(dirent) u_strlen((dirent)->d_name)
#  endif
#else
#  define dirent direct
#  define NAMLEN(dirent) (dirent)->d_namlen
#  if HAVE_SYS_NDIR_H
#     include <sys/ndir.h>
#  endif
#  if HAVE_SYS_DIR_H
#     include <sys/dir.h>
#  endif
#  if HAVE_NDIR_H
#     include <ndir.h>
#  endif
#endif

#ifndef DT_UNKNOWN
#define DT_UNKNOWN  0
#endif
#ifndef DT_DIR
#define DT_DIR      4
#endif
#ifndef DT_REG
#define DT_REG      8
#endif
#ifndef DT_LNK
#define DT_LNK     10
#endif

#ifdef _DIRENT_HAVE_D_TYPE
#define U_DT_TYPE dp->d_type
#else
#define U_DT_TYPE DT_UNKNOWN
#endif

#ifdef HAVE_FNMATCH
#  include <fnmatch.h>
#endif

#ifndef __MINGW32__
#  include <pwd.h>
#endif

#ifndef FNM_CASEFOLD
#define FNM_CASEFOLD FNM_IGNORECASE
#endif

#ifndef FNM_LEADING_DIR
#define FNM_LEADING_DIR FNM_PERIOD
#endif

/* Match */

int        u_pfn_flags;
bPFpcupcud u_pfn_match = u_dosmatch;

/* Services */

const char* u_short_units[4] = { "B/s", "K/s", "M/s", "G/s" };

const char* u_getPathRelativ(const char* path, uint32_t* ptr_path_len)
{
   uint32_t path_len = *ptr_path_len;

   U_INTERNAL_TRACE("u_getPathRelativ(%s,%u)", path, *ptr_path_len)

   U_INTERNAL_ASSERT_POINTER(path)
   U_INTERNAL_ASSERT_POINTER(u_cwd)

   U_INTERNAL_PRINT("u_cwd = %s", u_cwd)

   if (path[0] == u_cwd[0])
      {
      while (path[path_len-1] == '/') --path_len;

      if (path_len >= u_cwd_len &&
          memcmp(path, u_cwd, u_cwd_len) == 0)
         {
         if (path_len == u_cwd_len)
            {
            path     = ".";
            path_len = 1;
            }
         else
            {
            uint32_t len = u_cwd_len + (u_cwd_len > 1);

            path     += len;
            path_len -= len;
            }
         }
      }

   if (path[0] == '.' &&
       path[1] == '/')
      {
      path     += 2;
      path_len -= 2;
      }

   while (path[0] == '/' &&
          path[1] == '/')
      {
      ++path;
      --path_len;
      }

   *ptr_path_len = path_len;

   U_INTERNAL_PRINT("path(%u) = %.*s", path_len, path_len, path)

   return path;
}

/* find sequence of U_LF2 or U_CRLF2 */

uint32_t u_findEndHeader(const char* str, uint32_t n)
{
   const char* p;
   const char* end = str + n;
   const char* ptr = str;

   uint32_t endHeader = U_NOT_FOUND;

   U_INTERNAL_TRACE("u_findEndHeader(%.*s,%u)", U_min(n,128), str, n)

   U_INTERNAL_ASSERT_POINTER(str)

   while (ptr < end)
      {
      p = (const char*) memchr(ptr, '\n', end - ptr);

      if (p == NULL) break;

      // \n\n

      if (p[1] == '\n')
         {
         endHeader = p - str + 2;

         u_line_terminator     = U_LF;
         u_line_terminator_len = 1;

         break;
         }

      // \r\n\r\n

      if (p[-1] == '\r' &&
          p[1]  == '\r' &&
          p[2]  == '\n')
         {
         endHeader = p - str + 3;

         u_line_terminator     = U_CRLF;
         u_line_terminator_len = 2;

         break;
         }

      ptr = p + 1;
      }

   if (endHeader > n) return U_NOT_FOUND;

   return endHeader;
}

/* Change the current working directory to the `user` user's home dir, and downgrade security to that user account */

bool u_ranAsUser(const char* user, bool change_dir)
{
#ifdef __MINGW32__
   return false;
#else
   struct passwd* pw;

   U_INTERNAL_TRACE("u_ranAsUser(%s,%d)", user, change_dir)

   U_INTERNAL_ASSERT_POINTER(user)

   if (!(pw = getpwnam(user)) ||
       setgid(pw->pw_gid)     ||
       setuid(pw->pw_uid))
      {
      return false;
      }

   /* change user name */

   (void) strncpy(u_user_name, user, (u_user_name_len = u_strlen(user)));

   if (change_dir &&
       pw->pw_dir &&
       pw->pw_dir[0])
      {
      (void) chdir(pw->pw_dir);

      u_getcwd(); /* get current working directory */

      U_INTERNAL_ASSERT_EQUALS(strcmp(pw->pw_dir,u_cwd),0)
      }

   return true;
#endif
}

/* Determine the width of the terminal we're running on */

int u_getScreenWidth(void)
{
#ifdef TIOCGWINSZ
   struct winsize wsz;
#endif

   U_INTERNAL_TRACE("u_getScreenWidth()", 0)

   /* If there's a way to get the terminal size using POSIX tcgetattr(), somebody please tell me. */

#ifdef TIOCGWINSZ
   if (ioctl(STDERR_FILENO, TIOCGWINSZ, &wsz) != -1)  /* most likely ENOTTY */
      {
      U_INTERNAL_PRINT("wsz.ws_col = %d", wsz.ws_col)

      return wsz.ws_col;
      }
#endif

   return 0;
}

/*
Calculate the download rate and trim it as appropriate for the speed. Appropriate means that
if rate is greater than 1K/s, kilobytes are used, and if rate is greater than 1MB/s, megabytes are used.
UNITS is zero for B/s, one for KB/s, two for MB/s, and three for GB/s
*/

double u_calcRate(uint64_t bytes, uint32_t msecs, int* units)
{
   double dlrate = (double)1000. * bytes / (double)msecs;

   U_INTERNAL_TRACE("u_calcRate(%u,%u,%p)", bytes, msecs, units)

   U_INTERNAL_ASSERT_POINTER(units)
   U_INTERNAL_ASSERT_MAJOR(bytes,0)
   U_INTERNAL_ASSERT_MAJOR(msecs,0)

   if      (dlrate < 1024.0)                   *units = 0;
   else if (dlrate < 1024.0 * 1024.0)          *units = 1, dlrate /=  1024.0;
   else if (dlrate < 1024.0 * 1024.0 * 1024.0) *units = 2, dlrate /= (1024.0 * 1024.0);
   else                                        *units = 3, dlrate /= (1024.0 * 1024.0 * 1024.0);

   U_INTERNAL_PRINT("dlrate = %7.2f%s", dlrate, u_short_units[*units])

   return dlrate;
}

void u_printSize(char* buffer, uint64_t bytes)
{
   int units;
   double size;

   U_INTERNAL_TRACE("u_printSize(%p,%llu)", buffer, bytes)

   if (bytes == 0)
      {
      (void) strcpy(buffer, "0 Byte");

      return;
      }

   size = u_calcRate(bytes, 1000, &units);

   if (units) (void) sprintf(buffer, "%5.2f %cBytes", size, u_short_units[units][0]);
   else       (void) sprintf(buffer, "%7.0f Bytes", size);
}

bool u_rmatch(const char* haystack, uint32_t haystack_len, const char* needle, uint32_t needle_len)
{
   U_INTERNAL_TRACE("u_rmatch(%.*s,%u,%.*s,%u)", U_min(haystack_len,128), haystack, haystack_len,
                                                 U_min(  needle_len,128),  needle,   needle_len)

   U_INTERNAL_ASSERT_POINTER(needle)
   U_INTERNAL_ASSERT_POINTER(haystack)
   U_INTERNAL_ASSERT_MAJOR(haystack_len,0)

   if (haystack_len >= needle_len)
      {
      // see if substring characters match at end

      const char* nn  = needle   + needle_len   - 1;
      const char* hh  = haystack + haystack_len - 1;

      while (*nn-- == *hh--)
         {
         if (nn >= needle) continue;

         return true; // we got all the way to the start of the substring so we must've won
         }
      }

   return false;
}

void* u_find(const char* s, uint32_t n, const char* a, uint32_t n1)
{
#ifdef HAVE_MEMMEM
   void* p;
#else
   uint32_t pos = 0;
#endif

   U_INTERNAL_TRACE("u_find(%.*s,%u,%.*s,%u)", U_min(n,128), s, n, U_min(n1,128), a, n1)

   U_INTERNAL_ASSERT_POINTER(s)
   U_INTERNAL_ASSERT_POINTER(a)
   U_INTERNAL_ASSERT_MAJOR(n,0)
   U_INTERNAL_ASSERT_MAJOR(n1,0)

   if (n1 == 1) return (void*) memchr(s, *a, n);

#ifdef HAVE_MEMMEM
   p = memmem(s, n, a, n1);

   U_INTERNAL_PRINT("memmem() = %p", p)

   return p;
#else
   for (; (pos + n1) <= n; ++pos) if (memcmp(s + pos, a, n1) == 0) return (void*)(s+pos);
#endif

   return 0;
}

/* Search a string for any of a set of characters.
 * Locates the first occurrence in the string s of any of the characters in the string accept
 */
 
const char* u_strpbrk(const char* s, uint32_t slen, const char* accept)
{
   const char* c;
   const char* end = s + slen;

   U_INTERNAL_TRACE("u_strpbrk(%.*s,%u,%s)", U_min(slen,128), s, slen, accept)

   U_INTERNAL_ASSERT_POINTER(s)
   U_INTERNAL_ASSERT_MAJOR(slen,0)
   U_INTERNAL_ASSERT_POINTER(accept)

   while (s < end)
      {
      for (c = accept; *c; ++c)
         {
         if (*s == *c) return s;
         }

      ++s;
      }

   return 0;
}

/* Search a string for a terminator of a group of delimitator {} [] () <%%>...*/

const char* u_strpend(const char* s, uint32_t slen, const char* group_delimitor, uint32_t group_delimitor_len, char skip_line)
{
   char c;
   int level = 1;
   const char* end = s + slen;
   uint32_t i, n = group_delimitor_len / 2;

   U_INTERNAL_TRACE("u_strpend(%.*s,%u,%s,%u,%c)", U_min(slen,128), s, slen, group_delimitor, group_delimitor_len, skip_line)

   U_INTERNAL_ASSERT_POINTER(s)
   U_INTERNAL_ASSERT_MAJOR(slen,0)
   U_INTERNAL_ASSERT_POINTER(group_delimitor)
   U_INTERNAL_ASSERT_EQUALS(s[0], group_delimitor[n-1])
   U_INTERNAL_ASSERT_EQUALS(group_delimitor_len & 1, 0)

loop:
   c = *++s;

   if (s >= end) goto done;

   if (u_isspace(c)) goto loop;

   if (c == skip_line) U_SKIP_LINE_COMMENT(s,end,loop)

   else if (c == group_delimitor[0] && *(s-1) != '\\')
      {
      U_INTERNAL_PRINT("c = %c level = %d s = %.*s", c, level, 10, s)

      for (i = 1; i < n; ++i) if (s[i] != group_delimitor[i]) goto loop;

      ++level;
      }
   else if (c == group_delimitor[n] && *(s-1) != '\\')
      {
      U_INTERNAL_PRINT("c = %c level = %d s = %.*s", c, level, 10, s)

      for (i = 1; i < n; ++i) if (s[i] != group_delimitor[n+i]) goto loop;

      if (--level == 0) return s;
      }

   U_INTERNAL_PRINT("level = %d s = %.*s", level, 10, s)

   goto loop;

done:
   return 0;
}

/* check if string a start with string b */

bool u_startsWith(const char* a, uint32_t n1, const char* b, uint32_t n2)
{
   int32_t diff = n1 - n2;

   U_INTERNAL_TRACE("u_startsWith(%.*s,%u,%.*s,%u)", U_min(n1,128), a, n1, U_min(n2,128), b, n2)

   if (diff >= 0 &&
       (strncmp(a, b, n2) == 0))
      {
      return true;
      }

   return false;
}

/* check if string a terminate with string b */

bool u_endsWith(const char* a, uint32_t n1, const char* b, uint32_t n2)
{
   int32_t diff = n1 - n2;

   U_INTERNAL_TRACE("u_endsWith(%.*s,%u,%.*s,%u)", U_min(n1,128), a, n1, U_min(n2,128), b, n2)

   if (diff >= 0 &&
       (strncmp(a+diff, b, n2) == 0))
      {
      return true;
      }

   return false;
}

bool u_isNumber(const char* s, uint32_t n)
{
   int vdigit[]    = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0 };
   const char* end = s + n;

   U_INTERNAL_TRACE("u_isNumber(%.*s,%u)", U_min(n,128), s, n)

   U_INTERNAL_ASSERT_POINTER(s)
   U_INTERNAL_ASSERT_MAJOR(n,0)

   if (*s == '+' ||
       *s == '-')
      {
      ++s;
      }

   while (s < end &&
          ((*(const unsigned char*)s) >> 4) == 0x03 &&
          vdigit[(*(const unsigned char*)s)  & 0x0f])
      {
      U_INTERNAL_PRINT("*s = %c, *s >> 4 = %c ", *s, (*(char*)s) >> 4)

      ++s;
      }

   return (s == end);
}

/* Match STRING against the filename pattern MASK, returning true if it matches, false if not */

bool u_dosmatch(const char* s, uint32_t n1, const char* mask, uint32_t n2, int ignorecase)
{
   const char* cp = 0;
   const char* mp = 0;
   unsigned char c1, c2;

   const char* end_s    =    s + n1;
   const char* end_mask = mask + n2;

   U_INTERNAL_TRACE("u_dosmatch(%.*s,%u,%.*s,%u,%d)", U_min(n1,128), s, n1, n2, mask, n2, ignorecase)

   U_INTERNAL_ASSERT_POINTER(s)
   U_INTERNAL_ASSERT_POINTER(mask)
   U_INTERNAL_ASSERT_MAJOR(n1,0)
   U_INTERNAL_ASSERT_MAJOR(n2,0)

   if (ignorecase)
      {
      while (s < end_s)
         {
         c2 = u_tolower(*mask);

         if (c2 == '*') break;

         c1 = u_tolower(*s);

         if (c2 != c1 &&
             c2 != '?') return false;

         ++s;
         ++mask;
         }

      U_INTERNAL_PRINT("c1 = %c c2 = %c", c1, c2)

      while (true)
         {
         if (s >= end_s)
            {
            while (*mask == '*') ++mask;

            return (mask >= end_mask);
            }

         c2 = u_tolower(*mask);

         if (c2 == '*')
            {
            if (++mask >= end_mask) return true;

            cp = s + 1;
            mp = mask;

            continue;
            }

         c1 = u_tolower(*s);

         U_INTERNAL_PRINT("c1 = %c c2 = %c", c1, c2)

         if (c2 == c1 ||
             c2 == '?')
            {
            ++s;
            ++mask;

            continue;
            }

         s    = cp++;
         mask = mp;
         }
      }
   else
      {
      while (s < end_s)
         {
         c2 = *mask;

         if (c2 == '*') break;

         c1 = *s;

         if (c2 != c1 &&
             c2 != '?') return false;

         ++s;
         ++mask;
         }

      U_INTERNAL_PRINT("c1 = %c c2 = %c", c1, c2)

      while (true)
         {
         if (s >= end_s)
            {
            while (*mask == '*') ++mask;

            return (mask >= end_mask);
            }

         c2 = *mask;

         if (c2 == '*')
            {
            if (++mask >= end_mask) return true;

            cp = s + 1;
            mp = mask;

            continue;
            }

         c1 = *s;

         U_INTERNAL_PRINT("c1 = %c c2 = %c", c1, c2)

         if (c2 == c1 ||
             c2 == '?')
            {
            ++s;
            ++mask;

            continue;
            }

         s    = cp++;
         mask = mp;
         }
      }
}

/* Match STRING against the filename pattern MASK and multiple patterns separated by '|', returning true if it matches, false if not */

bool u_dosmatch_with_OR(const char* s, uint32_t n1, const char* mask, uint32_t n2, int ignorecase)
{
   const char* or;

   U_INTERNAL_TRACE("u_dosmatch_with_OR(%.*s,%u,%.*s,%u,%d)", U_min(n1,128), s, n1, n2, mask, n2, ignorecase)

   U_INTERNAL_ASSERT_POINTER(s)
   U_INTERNAL_ASSERT_POINTER(mask)
   U_INTERNAL_ASSERT_MAJOR(n1,0)
   U_INTERNAL_ASSERT_MAJOR(n2,0)

   while (true)
      {
      or = (const char*) memchr(mask, '|', n2);

      if (or == NULL) return u_dosmatch(s, n1, mask, n2, ignorecase);

      n2 = or - mask;

      if (u_dosmatch(s, n1, mask, n2, ignorecase)) return true;

      mask = or + 1;
      }
}

bool u_isMacAddr(const char* p, uint32_t len)
{
   uint32_t c;

   U_INTERNAL_TRACE("u_isMacAddr(%.*s,%u)", U_min(len,128), p, len)

   /* cisco-style: 0123.4567.89ab */

   if (3*5-1 == len)
      {
      for (c = 0; c < len; ++c)
         {
         if ((c % 5) == 4)
            {
            if (p[c] != '.') return false;
            }
         else
            {
            if (!u_isxdigit(p[c])) return false;
            }
         }

      return true;
      }

   /* windows-style: 01-23-45-67-89-ab, 01:23:45:67:89:ab */

   if (6*3-1 == len)
      {
      for (c = 0; c < len; ++c)
         {
         if ((c % 3) == 2)
            {
            if (p[c] != ':' &&
                p[c] != '-') return false;
            }
         else
            {
            if (!u_isxdigit(p[c])) return false;
            }
         }

      return true;
      }

   return false;
}

/* Verifies that the passed string is actually an e-mail address
 * see also: http://www.remote.org/jochen/mail/info/chars.html
 */

#define RFC822_SPECIALS "()<>@,;:\\\"[]"

bool u_validate_email_address(const char* address, uint32_t address_len)
{
   int count;
   const char* c;
   const char* end;
   const char* domain;

   U_INTERNAL_TRACE("u_validate_email_address(%.*s,%u)", U_min(address_len,128), address, address_len)

   if (address_len < 3) return false;

   /* first we validate the name portion (name@domain) */

   for (c = address, end = address + address_len; c < end; ++c)
      {
      U_INTERNAL_PRINT("c = %c", *c)

      if (*c == '\"' &&
          (c == address || *(c - 1) == '.' || *(c - 1) == '\"'))
         {
         while (++c < end)
            {
            U_INTERNAL_PRINT("c = %c", *c)

            if (*c == '\"') break;

            if (*c == '\\' && (*++c == ' ')) continue;

            if (*c <= ' ' || *c >= 127) return false;
            }

         if (c++ >= end) return false;

         U_INTERNAL_PRINT("c = %c", *c)

         if (*c == '@') break;

         if (*c != '.') return false;

         continue;
         }

      if (*c == '@') break;

      if (*c <= ' ' ||
          *c >= 127) return false;

      if (strchr(RFC822_SPECIALS, *c)) return false;
      }

   if (c == address || *(c - 1) == '.') return false;

   /* next we validate the domain portion (name@domain) */

   if ((domain = ++c) >= end) return false;

   count = 0;

   do {
      U_INTERNAL_PRINT("c = %c", *c)

      if (*c == '.')
         {
         if (c == domain || *(c - 1) == '.') return false;

         ++count;
         }

      if (*c <= ' ' ||
          *c >= 127) return false;

      if (strchr(RFC822_SPECIALS, *c)) return false;
      }
   while (++c < end);

   return (count >= 1);
}

/* Perform 'natural order' comparisons of strings. */

int u_strnatcmp(char const* a, char const* b)
{
   char ca, cb;
   int ai = 0, bi = 0;

   U_INTERNAL_TRACE("u_strnatcmp(%s,%s)", a, b)

   U_INTERNAL_ASSERT_POINTER(a)
   U_INTERNAL_ASSERT_POINTER(b)

   while (true)
      {
      ca = a[ai];
      cb = b[bi];

      /* skip over leading spaces or zeros */
      while (u_isspace(ca) || ca == '0') ca = a[++ai];
      while (u_isspace(cb) || cb == '0') cb = b[++bi];

      /* process run of digits */
      if (u_isdigit(ca) &&
          u_isdigit(cb))
         {
         int bias = 0;

         /* The longest run of digits (stripping off leading zeros) wins. That aside, the greatest value
         wins, but we can't know that it will until we've scanned both numbers to know that they have the
         same magnitude, so we remember it in BIAS */

         while (true)
            {
            if (!u_isdigit(ca) &&
                !u_isdigit(cb))
               {
               goto done_number;
               }

            else if (!u_isdigit(ca)) return -1;
            else if (!u_isdigit(cb)) return  1;
            else if (ca < cb)
               {
               if (!bias) bias = -1;
               }
            else if (ca > cb)
               {
               if (!bias) bias = 1;
               }
            else if (!ca &&
                     !cb)
               {
               return bias;
               }

            ca = a[++ai];
            cb = b[++bi];
            }

done_number:

         if (bias) return bias;
         }

      if (!ca &&
          !cb)
         {
         /* The strings compare the same. Perhaps the caller will want to call strcmp to break the tie. */

         return 0;
         }

      /*
      if (fold_case)
         {
         ca = u_toupper(ca);
         cb = u_toupper(cb);
         }
      */

      if      (ca < cb) return -1;
      else if (ca > cb) return  1;

      ++ai;
      ++bi;
      }
}

uint32_t u_split(char* s, uint32_t n, char* argv[], const char* delim)
{
   char c;
   char* p;
   char* end  = s + n;
   char** ptr = argv;

   U_INTERNAL_TRACE("u_split(%.*s,%u,%p,%s)", U_min(n,128), s, n, argv, delim)

   U_INTERNAL_ASSERT_POINTER(s)
   U_INTERNAL_ASSERT_MAJOR(n,0)
   U_INTERNAL_ASSERT_POINTER(argv)
   U_INTERNAL_ASSERT_EQUALS(u_isBinary((const unsigned char*)s,n),false)

loop:
   if (delim)
      {
      U_SKIP_EXT(s,end,loop,done,delim)

      U_DELIMIT_TOKEN_EXT(s,end,p,delim)
      }
   else
      {
      U_SKIP(s,end,loop,done)

      U_DELIMIT_TOKENC(c,s,end,p)
      }

   U_INTERNAL_PRINT("s = %.*s", 20, s)

   if (s <= end)
      {
      *argv++ = p;

      *s++ = '\0';

      U_INTERNAL_PRINT("u_split() = %s", p)

      goto loop;
      }

done:
   *argv = 0;

   n = (argv - ptr);

   return n;
}

#ifdef __MINGW32__
#  define PATH_LIST_SEP ';'
#else
#  define PATH_LIST_SEP ':'
#endif

/* Given a string containing units of information separated by colons, return the next one pointed to by (P_INDEX), or NULL if there are no more.
 * Advance (P_INDEX) to the character after the colon
 */

static inline char* extract_colon_unit(char* pzDir, const char* string, uint32_t string_len, uint32_t* p_index)
{
         char* pzDest = pzDir;
   const char* pzSrc  = string + *p_index;

   U_INTERNAL_TRACE("extract_colon_unit(%s,%.*s,%u,%p)", pzDir, string_len, string, string_len, p_index)

   if ((string == NULL) ||
       (*p_index >= string_len))
      {
      return 0;
      }

   while (*pzSrc == PATH_LIST_SEP) pzSrc++;

   for (;;)
      {
      char ch = (*pzDest = *pzSrc);

      if (ch == '\0') break;

      if (ch == PATH_LIST_SEP)
         {
         *pzDest = '\0';

         break;
         }

      pzDest++;
      pzSrc++;
      }

   if (*pzDir == '\0') return 0;

   *p_index = (pzSrc - string);

   return pzDir;
}

/* Turn STRING (a pathname) into an absolute pathname, assuming that DOT_PATH contains the symbolic location of '.' */

static inline void make_absolute(char* result, const char* dot_path, const char* string)
{
   int result_len;

   U_INTERNAL_TRACE("make_absolute(%p,%s,%s)", result, dot_path, string)

   U_INTERNAL_ASSERT_POINTER(dot_path)

   if (dot_path[0])
      {
      strcpy(result, dot_path);

      result_len = u_strlen(result);

      if (result[result_len - 1] != PATH_SEPARATOR)
         {
         result[result_len++] = PATH_SEPARATOR;
         result[result_len]   = '\0';
         }
      }
   else
      {
      result[0] = '.';
      result[1] = PATH_SEPARATOR;
      result[2] = '\0';

      result_len = 2;
      }

   strcpy(result + result_len, string);
}

/* --------------------------------------------------------------------------------------------------------------------------------------------------------
// find a FILE MODE along PATH
// --------------------------------------------------------------------------------------------------------------------------------------------------------
// pathfind looks for a a file with name FILENAME and MODE access along colon delimited PATH, and build the full pathname as a string, or NULL if not found
// --------------------------------------------------------------------------------------------------------------------------------------------------------
*/

#ifdef __MINGW32__
#  define U_PATH_DEFAULT "C:\\msys\\1.0\\bin;C:\\MinGW\\bin;C:\\windows;C:\\windows\\system;C:\\windows\\system32"

static const char* u_check_for_suffix_exe(const char* program)
{
   static char program_w32[MAX_FILENAME_LEN + 1];

   int len = u_strlen(program);

   U_INTERNAL_TRACE("u_check_for_suffix_exe(%s)", program)

   if (u_endsWith(program, len, U_CONSTANT_TO_PARAM(".exe")) == false)
      {
      (void) memcpy(program_w32, program, len);
      (void) memcpy(program_w32+len, ".exe", sizeof(".exe"));

      program = program_w32;

      U_INTERNAL_PRINT("program = %s", program)
      }

   return program;
}
#else
#  define U_PATH_DEFAULT "/sbin:/usr/sbin:/usr/local/sbin:/bin:/usr/bin:/usr/local/bin"
#endif

bool u_pathfind(char* result, const char* path, uint32_t path_len, const char* filename, int mode)
{
   char* colon_unit;
   uint32_t p_index = 0;
   char zPath[U_PATH_MAX + 1];

   U_INTERNAL_TRACE("u_pathfind(%p,%.*s,%u,%s,%d)", result, path_len, path, path_len, filename, mode)

   if (path_len == 0)
      {
      path = getenv("PATH");

      if (path) path_len = u_strlen(path);
      else
         {
         path     = U_PATH_DEFAULT;
         path_len = U_CONSTANT_SIZE(U_PATH_DEFAULT);
         }

      U_INTERNAL_PRINT("path(%u) = %.*s", path_len, path_len, path)
      }

#ifdef __MINGW32__
   if (mode & X_OK) filename = u_check_for_suffix_exe(filename);
#endif

   /* FOR each non-null entry in the colon-separated path, DO ... */

   zPath[0] = '\0';

   while (true)
      {
      colon_unit = extract_colon_unit(zPath, path, path_len, &p_index);

      /* IF no more entries, THEN quit */

      if (colon_unit == NULL) break;

      make_absolute(result, colon_unit, filename);

      /* Make sure we can access it in the way we want */

      if (access(result, mode) >= 0)
         {
         /* We can, so normalize the name and return it below */

         u_canonicalize_pathname(result);

         return true;
         }
      }

   return false;
}

/* ----------------------------------------------------------------------------------
// Canonicalize PATH, and build a new path. The new path differs from PATH in that:
// ----------------------------------------------------------------------------------
// Multiple    '/'                     are collapsed to a single '/'
// Leading     './'  and trailing '/.' are removed
// Trailing    '/'                     are removed
// Trailing    '/.'                    are removed
// Non-leading '../' and trailing '..' are handled by removing portions of the path
// ---------------------------------------------------------------------------------- */

void u_canonicalize_pathname(char* path)
{
   int len;
   char* p;
   char* s;
   char* lpath = path;

   U_INTERNAL_TRACE("u_canonicalize_pathname(%s)", path)

#ifdef __MINGW32__
   if (u_isalpha(path[0]) && path[1] == ':') lpath += 2; /* Skip over the disk name in MSDOS pathnames */
#endif

   /* Collapse multiple slashes */

   for (p = lpath; *p; ++p)
      {
      if (p[0] == '/' &&
          p[1] == '/')
         {
         s = p + 1;

         while (*(++s) == '/');

         (void) strcpy(p + 1, s);
         }
      }

   /* Collapse "/./" -> "/" */

   p = lpath;

   while (*p)
      {
      if (p[0] == '/' &&
          p[1] == '.' &&
          p[2] == '/')
         {
         (void) strcpy(p, p + 2);
         }
      else
         {
         ++p;
         }
      }

   /* Remove trailing slashes */

   p = lpath + u_strlen(lpath) - 1;

   while (p > lpath && *p == '/') *p-- = '\0';

   /* Remove leading "./" */

   if (lpath[0] == '.' &&
       lpath[1] == '/')
      {
      if (lpath[2] == 0)
         {
         lpath[1] = 0;

         return;
         }

      (void) strcpy(lpath, lpath + 2);
      }

   /* Remove trailing "/" or "/." */

   len = u_strlen(lpath);

   if (len < 2) return;

   if (lpath[len - 1] == '/') lpath[len - 1] = 0;
   else
      {
      if (lpath[len - 1] == '.' && lpath[len - 2] == '/')
         {
         if (len == 2)
            {
            lpath[1] = 0;

            return;
            }

         lpath[len - 2] = 0;
         }
      }

   /* Collapse "/.." with the previous part of path */

   p = lpath;

   while (p[0] && p[1] && p[2])
      {
      if ((p[0] != '/' || p[1] != '.' || p[2] != '.') ||
          (p[3] != '/' && p[3] != 0))
         {
         ++p;

         continue;
         }

      /* search for the previous token */

      s = p - 1;

      while (s >= lpath && *s != '/') --s;

      ++s;

      /* If the previous token is "..", we cannot collapse it */

      if (s[0] == '.' && s[1] == '.' && (s + 2) == p)
         {
         p += 3;

         continue;
         }

      if (p[3] != 0)
         {
         /*      "/../foo" -> "/foo" */
         /* "token/../foo" ->  "foo" */

         (void) strcpy(s + (s == lpath && *s == '/'), p + 4);

         p = s - (s > lpath);

         continue;
         }

      /* trailing ".." */

      if (s == lpath)
         {
         /* "token/.." -> "." */

         if (lpath[0] != '/') lpath[0] = '.';

         lpath[1] = 0;
         }
      else
         {
         /* "foo/token/.." -> "foo" */

         if (s == (lpath + 1)) s[ 0] = '\0';
         else                  s[-1] = '\0';

         break;
         }

      break;
      }
}

/* Prepare command for call to exec() */

int u_splitCommand(char* s, uint32_t n, char* argv[], char* pathbuf, uint32_t pathbuf_size)
{
   char c;
   uint32_t i = 0;
   bool bpath = false;
   int result = u_split(s, n, argv+1, 0);

   U_INTERNAL_TRACE("u_splitCommand(%.*s,%u,%p,%p,%u)", U_min(n,128), s, n, argv, pathbuf, pathbuf_size)

   U_INTERNAL_ASSERT_POINTER(s)
   U_INTERNAL_ASSERT_MAJOR(n,0)
   U_INTERNAL_ASSERT_MAJOR(pathbuf_size,0)

   /* check if command have path separator... */

   while ((c = argv[1][i++]))
      {
      if (IS_DIR_SEPARATOR(c))
         {
         bpath = true;

         break;
         }
      }

   if (bpath)
      {
      argv[0] = argv[1];
      argv[1] = (char*) u_basename(argv[0]);

      pathbuf[0] = '\0';
      }
   else
      {
      argv[0] = pathbuf;

#  ifdef __MINGW32__
      argv[1] = (char*)u_check_for_suffix_exe(argv[1]);
#  endif

      if (u_pathfind(pathbuf, 0, 0, argv[1], R_OK | X_OK) == false) return -1;

      U_INTERNAL_ASSERT_MINOR(u_strlen(pathbuf), pathbuf_size)
      }

   return result;
}

/* FTW - walks through the directory tree starting from the indicated directory.
 *       For each found entry in the tree, it calls fn() with the full pathname of the entry, his length and if it is a directory
 *
 * struct dirent {
 *    ino_t          d_ino;       // inode number
 *    off_t          d_off;       // offset to the next dirent
 *    unsigned short d_reclen;    // length of this record
 *    unsigned char  d_type;      // type of file; not supported by all file system types
 *    char           d_name[256]; // filename
 * };
 *
 * struct u_ftw_ctx_s {
 *    qcompare sort_by;
 *    const char* filter;
 *    vPF call, call_if_up;
 *    uint32_t filter_len;
 *    bool depth, is_directory, call_if_directory;
 *
 *    -----------------------
 *    NB: we use u_buffer...
 *    -----------------------
 *    uint32_t pathlen;
 *    char*    fullpath;
 *    -----------------------
 * };
 */

struct u_dirent_s {
   ino_t         d_ino;
   uint32_t      d_name, d_namlen;
   unsigned char d_type;
};

struct u_dir_s {
   char* free;
   struct u_dirent_s* dp;
   uint32_t num, max, pfree, nfree, szfree;
};

struct u_ftw_ctx_s u_ftw_ctx;

#define U_DIRENT_ALLOCATE   (128U * 1024U)
#define U_FILENAME_ALLOCATE ( 32U * U_DIRENT_ALLOCATE)

static inline void u_ftw_allocate(struct u_dir_s* u_dir)
{
   U_INTERNAL_TRACE("u_ftw_allocate(%p)", u_dir)

   if (u_ftw_ctx.sort_by)
      {
      u_dir->free  = (char*) malloc((u_dir->nfree = u_dir->szfree = U_FILENAME_ALLOCATE));
      u_dir->dp    = (struct u_dirent_s*) malloc((u_dir->max = U_DIRENT_ALLOCATE) * sizeof(struct u_dirent_s));
      u_dir->pfree = 0U;
      }

   u_dir->num = 0U;
}

static inline void u_ftw_deallocate(struct u_dir_s* u_dir)
{
   U_INTERNAL_TRACE("u_ftw_deallocate(%p)", u_dir)

   if (u_ftw_ctx.sort_by)
      {
      free(u_dir->free);
      free(u_dir->dp);
      }
}

static inline void u_ftw_reallocate(struct u_dir_s* u_dir, uint32_t d_namlen)
{
   U_INTERNAL_TRACE("u_ftw_reallocate(%p,%u)", u_dir, d_namlen)

   U_INTERNAL_ASSERT_DIFFERS(u_ftw_ctx.sort_by,0)

   if (u_dir->num >= u_dir->max)
      {
      u_dir->max += U_DIRENT_ALLOCATE;

      U_INTERNAL_PRINT("Reallocating u_dir->dp to size %u", u_dir->max)

      u_dir->dp = (struct u_dirent_s*) realloc(u_dir->dp, u_dir->max * sizeof(struct u_dirent_s));

      U_INTERNAL_ASSERT_POINTER(u_dir->dp)
      }

   if (d_namlen > u_dir->nfree)
      {
      u_dir->nfree  += U_FILENAME_ALLOCATE;
      u_dir->szfree += U_FILENAME_ALLOCATE;

      U_INTERNAL_PRINT("Reallocating u_dir->free to size %u", u_dir->szfree)

      u_dir->free = (char*) realloc(u_dir->free, u_dir->szfree);

      U_INTERNAL_ASSERT_POINTER(u_dir->free)
      }
}

static void u_ftw_call(char* d_name, uint32_t d_namlen, unsigned char d_type)
{
   U_INTERNAL_TRACE("u_ftw_call(%.*s,%u,%d)", d_namlen, d_name, d_namlen, d_type)

   if (d_type != DT_REG &&
       d_type != DT_DIR &&
       d_type != DT_LNK &&
       d_type != DT_UNKNOWN) return;

   (void) memcpy(u_buffer + u_buffer_len, d_name, d_namlen);

   u_buffer_len += d_namlen;

   U_INTERNAL_ASSERT_MINOR(u_buffer_len,4096)

   u_buffer[u_buffer_len] = '\0';

   u_ftw_ctx.is_directory = (d_type == DT_DIR);

   if ( u_ftw_ctx.depth &&
       (u_ftw_ctx.is_directory ||
        d_type == DT_UNKNOWN))
      {
      u_ftw();

      goto end;
      }

   u_ftw_ctx.call();

end:
   u_buffer_len -= d_namlen;
}

int u_ftw_ino_cmp(const void* a, const void* b) { return (((const struct u_dirent_s*)a)->d_ino - ((const struct u_dirent_s*)b)->d_ino); }

static void u_ftw_readdir(DIR* dirp)
{
   struct dirent* dp;
   uint32_t i, d_namlen;
   struct u_dir_s u_dir;
   struct u_dirent_s* ds;

   U_INTERNAL_TRACE("u_ftw_readdir(%p)", dirp)

   u_ftw_allocate(&u_dir);

   /* -----------------------------------------
    * NB: NON sono sempre le prime due entry !!
    * -----------------------------------------
    * (void) readdir(dirp);         // skip '.'
    * (void) readdir(dirp);         // skip '..'
    * -----------------------------------------
    */

   while ((dp = (struct dirent*) readdir(dirp)))
      {
      d_namlen = NAMLEN(dp);

      U_INTERNAL_PRINT("d_namlen = %u d_name = %.*s", d_namlen, d_namlen, dp->d_name)

      if (U_ISDOTS(dp->d_name)) continue;

      if (u_ftw_ctx.filter == 0 ||
          u_pfn_match(dp->d_name, d_namlen, u_ftw_ctx.filter, u_ftw_ctx.filter_len, u_pfn_flags))
         {
         if (u_ftw_ctx.sort_by)
            {
            u_ftw_reallocate(&u_dir, d_namlen); /* check se necessarie nuove allocazioni */

            ds = u_dir.dp + u_dir.num++;

            ds->d_ino  = dp->d_ino;
            ds->d_type = U_DT_TYPE;

            (void) memcpy(u_dir.free + (ds->d_name = u_dir.pfree), dp->d_name, (ds->d_namlen = d_namlen));

            u_dir.pfree += d_namlen;
            u_dir.nfree -= d_namlen;

            U_INTERNAL_PRINT("readdir: %lu %.*s %d", ds->d_ino, ds->d_namlen, u_dir.free + ds->d_name, ds->d_type);
            }
         else
            {
            u_ftw_call(dp->d_name, d_namlen, U_DT_TYPE);
            }
         }
      }

   (void) closedir(dirp);

   U_INTERNAL_PRINT("u_dir.num = %u", u_dir.num)

   if (u_dir.num)
      {
      qsort(u_dir.dp, u_dir.num, sizeof(struct u_dirent_s), u_ftw_ctx.sort_by);

      for (i = 0; i < u_dir.num; ++i)
         {
         ds = u_dir.dp + i;

         u_ftw_call(u_dir.free + ds->d_name, ds->d_namlen, ds->d_type);
         }
      }

   u_ftw_deallocate(&u_dir);
}

void u_ftw(void)
{
   DIR* dirp;

   U_INTERNAL_TRACE("u_ftw()", 0)

   U_INTERNAL_ASSERT_EQUALS(strlen(u_buffer), u_buffer_len)

   /*
    * NB: if is present the char 'filetype' this item isn't a directory and we don't need to try opendir()...
    *
    * dirp = (DIR*) (u_ftw_ctx.filetype && strchr(u_buffer+1, u_ftw_ctx.filetype) ? 0 : opendir(u_buffer));
    */

   u_ftw_ctx.is_directory = ((dirp = (DIR*) opendir(u_buffer)) != 0);

   if (u_ftw_ctx.is_directory == false ||
       u_ftw_ctx.call_if_directory)
      {
      u_ftw_ctx.call();
      }

   if (u_ftw_ctx.is_directory)
      {
      u_buffer[u_buffer_len++] = '/';

      u_ftw_readdir(dirp);

      --u_buffer_len;

      if (u_ftw_ctx.call_if_up) u_ftw_ctx.call_if_up(); /* for UTree<UString>::load() ... */
      }
}

int u_get_num_random(int range)
{
   static int random = 1;

   U_INTERNAL_TRACE("u_get_num_random(%d)", range)

   U_INTERNAL_ASSERT_MAJOR(range,0)

   random = (random * 1103515245 + 12345) & 0x7fffffff;

   return ((random % range) + 1);
}

#ifdef HAVE_SSL
int u_passwd_cb(char* buf, int size, int rwflag, void* password)
{
   U_INTERNAL_TRACE("u_passwd_cb(%p,%d,%d,%p)", buf, size, rwflag, password)

   (void) memcpy(buf, (char*)password, size);

   buf[size-1] = '\0';

   size = u_strlen(buf);

   U_INTERNAL_PRINT("buf(%d) = %.*s", size, U_min(size,128), buf)

   return size;
}
#endif

/*
 * Function fnmatch() as specified in POSIX 1003.2-1992, section B.6.
 * Compares a filename or pathname to a pattern.
 */

static const char* end_p;
static const char* end_s;

static inline int rangematch(const char* pattern, char test, int flags, char** newp)
{
   char c, c2;
   int negate, ok;

   U_INTERNAL_TRACE("rangematch(%.*s,%c)", end_p - pattern, pattern, test)

   /*
   * A bracket expression starting with an unquoted circumflex
   * character produces unspecified results (IEEE 1003.2-1992,
   * 3.13.2). This implementation treats it like '!', for
   * consistency with the regular expression syntax.
   * J.T. Conklin (conklin@ngai.kaleida.com)
   */

   if ((negate = (*pattern == '!' || *pattern == '^')) != 0) ++pattern;

   if (flags & FNM_CASEFOLD) test = u_tolower((unsigned char)test);

   /*
   * A right bracket shall lose its special meaning and represent
   * itself in a bracket expression if it occurs first in the list.
   * -- POSIX.2 2.8.3.2
   */

   ok = 0;
   c  = *pattern++;

   do {
      if (c == '\\' && !(flags & FNM_NOESCAPE)) c = *pattern++;

      U_INTERNAL_PRINT("c = %c test = %c", c, test)

      if (pattern > end_p) return (-1); /* if (c == EOS) return (RANGE_ERROR); */

      if (c == '/' && (flags & FNM_PATHNAME)) return (0);

      if (flags & FNM_CASEFOLD) c = u_tolower((unsigned char)c);

      if (*pattern == '-' && (c2 = *(pattern+1)) != ']' && (pattern+1) != end_p)
         {
         pattern += 2;

         if (c2 == '\\' && !(flags & FNM_NOESCAPE)) c2 = *pattern++;

         if (pattern > end_p) return (-1); /* if (c2 == EOS) return (RANGE_ERROR); */

         if (flags & FNM_CASEFOLD) c2 = u_tolower((unsigned char)c2);

         if (c <= test && test <= c2) ok = 1;
         }
      else if (c == test)
         {
         ok = 1;
         }
      }
   while ((c = *pattern++) != ']');

   *newp = (char*) pattern;

   return (ok == negate ? 0 : 1);
}

static inline int kfnmatch(const char* pattern, const char* string, int flags, int nesting)
{
   char* newp;
   char c, test;
   const char* stringstart;

   U_INTERNAL_TRACE("kfnmatch(%.*s,%.*s,%d,%d)", end_p - pattern, pattern, end_s - string, string, flags, nesting)

   if (nesting == 20) return (1);

   for (stringstart = string;;)
      {
      c = *pattern++;

      if (pattern > end_p)
         {
         if ((flags & FNM_LEADING_DIR) && *string == '/') return (0);

         return (string == end_s ? 0 : 1);
         }

      switch (c)
         {
         case '?':
            {
            if (string == end_s) return (1);

            if (*string == '/' && (flags & FNM_PATHNAME)) return (1);

            if (*string == '.' && (flags & FNM_PERIOD) &&
                (string == stringstart || ((flags & FNM_PATHNAME) && *(string - 1) == '/')))
               {
               return (1);
               }

            ++string;
            }
         break;

         case '*':
            {
            c = *pattern;

            /* Collapse multiple stars. */

            while (c == '*') c = *++pattern;

            if (*string == '.' && (flags & FNM_PERIOD) &&
                (string == stringstart || ((flags & FNM_PATHNAME) && *(string - 1) == '/')))
               {
               return (1);
               }

            /* Optimize for pattern with * at end or before /. */

            if (pattern == end_p) /* if (c == EOS) */
               {
               if (flags & FNM_PATHNAME)
                  {
                  return ((flags & FNM_LEADING_DIR) || memchr(string, '/', end_s - string) == NULL ? 0 : 1);
                  }
               else
                  {
                  return (0);
                  }
               }
            else if (c == '/' && flags & FNM_PATHNAME)
               {
               if ((string = (const char*)memchr(string, '/', end_s - string)) == NULL) return (1);

               break;
               }

            /* General case, use recursion. */

            while (string < end_s)
               {
               test = *string;

               if (!kfnmatch(pattern, string, flags & ~FNM_PERIOD, nesting + 1)) return (0);

               if (test == '/' && flags & FNM_PATHNAME) break;

               ++string;
               }

            return (1);
            }

         case '[':
            {
            if (string == end_s) return (1);

            if (*string == '/' && (flags & FNM_PATHNAME)) return (1);

            if (*string == '.' && (flags & FNM_PERIOD) &&
                (string == stringstart || ((flags & FNM_PATHNAME) && *(string - 1) == '/')))
               {
               return (1);
               }

            switch (rangematch(pattern, *string,  flags, &newp))
               {
               case -1: goto norm;
               case  1: pattern = newp; break;
               case  0: return (1);
               }

            ++string;
            }
         break;

         case '\\':
            {
            if (!(flags & FNM_NOESCAPE))
               {
               c = *pattern++;

               if (pattern > end_p) /* if ((c = *pattern++) == EOS) */
                  {
                  c = '\\';

                  --pattern;
                  }
               }
            }

         /* FALLTHROUGH */

         default:
norm:
            {
            if (c == *string)
               {
               }
            else if ((flags & FNM_CASEFOLD) && (u_tolower((unsigned char)c) == u_tolower((unsigned char)*string)))
               {
               }
            else
               {
               return (1);
               }

            string++;
            }
         break;
         }
      }

   /* NOTREACHED */
}

#define __FNM_FLAGS (FNM_PATHNAME | FNM_NOESCAPE | FNM_PERIOD | FNM_LEADING_DIR | FNM_CASEFOLD)

bool u_fnmatch(const char* string, uint32_t n1, const char* pattern, uint32_t n2, int flags)
{
   U_INTERNAL_TRACE("u_fnmatch(%.*s,%u,%.*s,%u,%d)", U_min(n1,128), string, n1, n2, pattern, n2, flags)

   U_INTERNAL_ASSERT_MAJOR(n1,0)
   U_INTERNAL_ASSERT_MAJOR(n2,0)
   U_INTERNAL_ASSERT_POINTER(string)
   U_INTERNAL_ASSERT_POINTER(pattern)
   U_INTERNAL_ASSERT_EQUALS((flags & ~__FNM_FLAGS),0)

   end_s = string  + n1;
   end_p = pattern + n2;

   return (kfnmatch(pattern, string, flags, 0) == 0);
}

#ifdef GCOV
#  define U_LOOP_STRING( exec_code ) { for (char c = *s; n--; c = *(++s)) { exec_code; }; }
#else
/* This is INCREDIBLY ugly, but fast. We break the string up into 8 byte units. On the first time through
// the loop we get the "leftover bytes" (strlen % 8). On every other iteration, we perform 8 BODY's so we
// handle all 8 bytes. Essentially, this saves us 7 cmp & branch instructions. If this routine is heavily
// used enough, it's worth the ugly coding
*/
#  define U_LOOP_STRING( exec_code ) {     \
   char c;                                 \
   uint32_t U_LOOP_CNT = (n + 8 - 1) >> 3; \
   switch (n & (8 - 1)) {                  \
      case 0:                              \
         do { { c = *s++; exec_code; }     \
      case 7: { c = *s++; exec_code; }     \
      case 6: { c = *s++; exec_code; }     \
      case 5: { c = *s++; exec_code; }     \
      case 4: { c = *s++; exec_code; }     \
      case 3: { c = *s++; exec_code; }     \
      case 2: { c = *s++; exec_code; }     \
      case 1: { c = *s++; exec_code; }     \
         } while (--U_LOOP_CNT); } }
#endif

bool u_isBase64(const char* s, uint32_t n)
{
   U_LOOP_STRING( if (u_isbase64(c) == false) return false )

   U_INTERNAL_TRACE("u_isBase64(%.*s,%u)", U_min(n,128), s, n)

   return true;
}

bool u_isWhiteSpace(const char* s, uint32_t n)
{
   U_LOOP_STRING( if (u_isspace(c) == false) return false )

   U_INTERNAL_TRACE("u_isWhiteSpace(%.*s,%u)", U_min(n,128), s, n)

   return true;
}

bool u_isText(const unsigned char* s, uint32_t n)
{
   U_LOOP_STRING( if (u_istext(c) == false) return false )

   U_INTERNAL_TRACE("u_isText(%.*s,%u)", U_min(n,128), s, n)

   return true;
}

/* From RFC 3986

#define U_URI_UNRESERVED  0 // ALPHA (%41-%5A and %61-%7A) DIGIT (%30-%39) '-' '.' '_' '~'
#define U_URI_PCT_ENCODED 1
#define U_URI_GEN_DELIMS  2 // ':' '/' '?' '#' '[' ']' '@'
#define U_URI_SUB_DELIMS  4 // '!' '$' '&' '\'' '(' ')' '*' '+' ',' ';' '='
*/

int u_uri_encoded_char_mask = (U_URI_PCT_ENCODED | U_URI_GEN_DELIMS | U_URI_SUB_DELIMS);

const unsigned char u_uri_encoded_char[256] = {
   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /* 0x00 - 0x0f */
   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /* 0x10 - 0x1f */
   1, 4, 1, 2, 4, 1, 4, 4, 4, 4, 4, 4, 4, 0, 0, 2,  /*  !"#$%&'()*+,-./ */
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 4, 1, 4, 1, 2,  /* 0123456789:;<=>? */
   2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* @ABCDEFGHIJKLMNO */
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 2, 1, 0,  /* PQRSTUVWXYZ[\]^_ */
   1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* `abcdefghijklmno */
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1,  /* pqrstuvwxyz{|}~  */
   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};

/* Text detection based on ascmagic.c from the file(1) utility */

#define F 0 /* character never appears in text */
#define T 1 /* character appears in plain ASCII text */
#define I 2 /* character appears in ISO-8859 text */
#define X 3 /* character appears in non-ISO extended ASCII (Mac, IBM PC) */

const unsigned char u_text_chars[256] = {
   /*                  BEL BS HT LF    FF CR    */
   F, F, F, F, F, F, F, T, T, T, T, F, T, T, F, F,  /* 0x0X */
   /*                              ESC          */
   F, F, F, F, F, F, F, F, F, F, F, T, F, F, F, F,  /* 0x1X */
   T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T,  /* 0x2X */
   T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T,  /* 0x3X */
   T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T,  /* 0x4X */
   T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T,  /* 0x5X */
   T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T,  /* 0x6X */
   T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, F,  /* 0x7X */
   /*            NEL                            */
   X, X, X, X, X, T, X, X, X, X, X, X, X, X, X, X,  /* 0x8X */
   X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,  /* 0x9X */
   I, I, I, I, I, I, I, I, I, I, I, I, I, I, I, I,  /* 0xaX */
   I, I, I, I, I, I, I, I, I, I, I, I, I, I, I, I,  /* 0xbX */
   I, I, I, I, I, I, I, I, I, I, I, I, I, I, I, I,  /* 0xcX */
   I, I, I, I, I, I, I, I, I, I, I, I, I, I, I, I,  /* 0xdX */
   I, I, I, I, I, I, I, I, I, I, I, I, I, I, I, I,  /* 0xeX */
   I, I, I, I, I, I, I, I, I, I, I, I, I, I, I, I   /* 0xfX */
};

#undef F
#undef T
#undef I
#undef X

/************************************************************************
 * From rfc2044: encoding of the Unicode values on UTF-8:               *
 *                                                                      *
 * UCS-4 range (hex.)           UTF-8 octet sequence (binary)           *
 * 0000 0000-0000 007F   0xxxxxxx                                       *
 * 0000 0080-0000 07FF   110xxxxx 10xxxxxx                              *
 * 0000 0800-0000 FFFF   1110xxxx 10xxxxxx 10xxxxxx                     *
 ************************************************************************/

bool u_isUTF8(const unsigned char* buf, uint32_t len)
{
   unsigned char c;
   bool gotone = false;
   uint32_t j, following;
   const unsigned char* end = buf + len;

   U_INTERNAL_TRACE("u_isUTF8(%.*s,%u)", U_min(n,128), s, n)

   U_INTERNAL_ASSERT_POINTER(buf)

   while (buf < end)
      {
      /*
       * UTF is a string of 1, 2, 3 or 4 bytes. The valid strings
       * are as follows (in "bit format"):
       *
       *    0xxxxxxx                                      valid 1-byte
       *    110xxxxx 10xxxxxx                             valid 2-byte
       *    1110xxxx 10xxxxxx 10xxxxxx                    valid 3-byte
       *    11110xxx 10xxxxxx 10xxxxxx 10xxxxxx           valid 4-byte
       *    ........
       */

      c = *buf++;

      if ((c & 0x80) == 0) /* 0xxxxxxx is plain ASCII */
         {
         /*
         * Even if the whole file is valid UTF-8 sequences,
         * still reject it if it uses weird control characters.
         */
         if (u_text_chars[c] != 1) return false;
         }
      else if ((c & 0x40) == 0) return false; /* 10xxxxxx never 1st byte */
      else
         {
         /* 11xxxxxx begins UTF-8 */

         if      ((c & 0x20) == 0) following = 1; /* 110xxxxx */
         else if ((c & 0x10) == 0) following = 2; /* 1110xxxx */
         else if ((c & 0x08) == 0) following = 3; /* 11110xxx */
         else if ((c & 0x04) == 0) following = 4; /* 111110xx */
         else if ((c & 0x02) == 0) following = 5; /* 1111110x */
         else                      return false;

         for (j = 0; j < following; j++)
            {
            if (buf >= end) return gotone;

            c = *buf++;

            if ((c & 0x80) == 0 ||
                (c & 0x40))
               {
               return false;
               }
            }

         gotone = true;
         }
      }

   return gotone;
}

int u_isUTF16(const unsigned char* buf, uint32_t len)
{
   uint32_t be, i, c;

   U_INTERNAL_TRACE("u_isUTF16(%.*s,%u)", U_min(n,128), s, n)

   if (len < 2) return 0;

   if      (buf[0] == 0xff && buf[1] == 0xfe) be = 0;
   else if (buf[0] == 0xfe && buf[1] == 0xff) be = 1;
   else                                       return 0;

   for(i = 2; i + 1 < len; i += 2)
      {
      c = (be ? buf[i+1] + 256 * buf[i]
              : buf[i]   + 256 * buf[i+1]);

      if (c == 0xfffe) return 0;

      if (c < 128 && u_text_chars[c] != 1) return 0;
      }

   return (1 + be);
}

/* Table for character type identification
 * Shorter definitions to make the data more compact
 */

#define C 0x01 /* Control character */
#define D 0x02 /* Digit */
#define L 0x04 /* Lowercase */
#define P 0x08 /* Punctuation */
#define S 0x10 /* Space */
#define U 0x20 /* Uppercase */
#define X 0x40 /* Hexadecimal */
#define B 0x80 /* Blank */

#define CS  (C | S)
#define LU  (L | U)
#define LX  (L | X)
#define UX  (U | X)
#define SB  (S | B)
#define CSB (C | S | B)

static const unsigned char ctype_tab[] = {
   0,
   C,  C,  C,  C,  C,  C,  C,  C,  C,  CSB,CS, CS, CS, CS, C,  C,
   C,  C,  C,  C,  C,  C,  C,  C,  C,  C,  C,  C,  C,  C,  C,  C,
   SB, P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,
   D,  D,  D,  D,  D,  D,  D,  D,  D,  D,  P,  P,  P,  P,  P,  P,
   P,  UX, UX, UX, UX, UX, UX, U,  U,  U,  U,  U,  U,  U,  U,  U,
   U,  U,  U,  U,  U,  U,  U,  U,  U,  U,  U,  P,  P,  P,  P,  P,
   P,  LX, LX, LX, LX, LX, LX, L,  L,  L,  L,  L,  L,  L,  L,  L,
   L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  P,  P,  P,  P,  C,

   /* Assume an ISO-1 character set */

   C,  C,  C,  C,  C,  C,  C,  C,  C,  C,  C,  C,  C,  C,  C,  C,
   C,  C,  C,  C,  C,  C,  C,  C,  C,  C,  C,  C,  C,  C,  C,  C,
   S,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,
   P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,
   U,  U,  U,  U,  U,  U,  U,  U,  U,  U,  U,  U,  U,  U,  U,  U,
   U,  U,  U,  U,  U,  U,  U,  P,  U,  U,  U,  U,  U,  U,  U,  LU,
   L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,
   L,  L,  L,  L,  L,  L,  L,  P,  L,  L,  L,  L,  L,  L,  L,  L
};

const unsigned char* u__ct_tab = ctype_tab + 1;

#undef C
#undef D
#undef L
#undef P
#undef S
#undef U
#undef X
#undef O
#undef B

#undef CS
#undef LU
#undef LX
#undef UX
#undef SB
#undef CSB

/* Table for converting to lower-case */

static const unsigned char lower_tab[] = {
   0,
   '\0',   '\01',  '\02',  '\03',  '\04',  '\05',  '\06',  '\07',
   '\010', '\011', '\012', '\013', '\014', '\015', '\016', '\017',
   '\020', '\021', '\022', '\023', '\024', '\025', '\026', '\027',
   '\030', '\031', '\032', '\033', '\034', '\035', '\036', '\037',
   ' ',    '!',    '\"',   '#',    '$',    '%',    '&',    '\'',
   '(',    ')',    '*',    '+',    ',',    '-',    '.',    '/',
   '0',    '1',    '2',    '3',    '4',    '5',    '6',    '7',
   '8',    '9',    ':',    ';',    '<',    '=',    '>',    '?',
   '@',    'a',    'b',    'c',    'd',    'e',    'f',    'g',
   'h',    'i',    'j',    'k',    'l',    'm',    'n',    'o',
   'p',    'q',    'r',    's',    't',    'u',    'v',    'w',
   'x',    'y',    'z',    '[',    '\\',   ']',    '^',    '_',
   '`',    'a',    'b',    'c',    'd',    'e',    'f',    'g',
   'h',    'i',    'j',    'k',    'l',    'm',    'n',    'o',
   'p',    'q',    'r',    's',    't',    'u',    'v',    'w',
   'x',    'y',    'z',    '{',    '|',    '}',    '~',    '\177',

   /* ISO-1 */

   0x80,   0x81,   0x82,   0x83,   0x84,   0x85,   0x86,   0x87,
   0x88,   0x89,   0x8a,   0x8b,   0x8c,   0x8d,   0x8e,   0x8f,
   0x90,   0x91,   0x92,   0x93,   0x94,   0x95,   0x96,   0x97,
   0x98,   0x99,   0x9a,   0x9b,   0x9c,   0x9d,   0x9e,   0x9f,
   0xa0,   0xa1,   0xa2,   0xa3,   0xa4,   0xa5,   0xa6,   0xa7,
   0xa8,   0xa9,   0xaa,   0xab,   0xac,   0xad,   0xae,   0xaf,
   0xb0,   0xb1,   0xb2,   0xb3,   0xb4,   0xb5,   0xb6,   0xb7,
   0xb8,   0xb9,   0xba,   0xbb,   0xbc,   0xbd,   0xbe,   0xbf,
   0xe0,   0xe1,   0xe2,   0xe3,   0xe4,   0xe5,   0xe6,   0xe7,
   0xe8,   0xe9,   0xea,   0xeb,   0xec,   0xed,   0xee,   0xef,
   0xf0,   0xf1,   0xf2,   0xf3,   0xf4,   0xf5,   0xf6,   0xd7,
   0xf8,   0xf9,   0xfa,   0xfb,   0xfc,   0xfd,   0xfe,   0xdf,
   0xe0,   0xe1,   0xe2,   0xe3,   0xe4,   0xe5,   0xe6,   0xe7,
   0xe8,   0xe9,   0xea,   0xeb,   0xec,   0xed,   0xee,   0xef,
   0xf0,   0xf1,   0xf2,   0xf3,   0xf4,   0xf5,   0xf6,   0xf7,
   0xf8,   0xf9,   0xfa,   0xfb,   0xfc,   0xfd,   0xfe,   0xff
};

const unsigned char* u__ct_tol = lower_tab + 1;

/* Table for converting to upper-case */

static const unsigned char upper_tab[] = {
   0,
   '\0',   '\01',  '\02',  '\03',  '\04',  '\05',  '\06',  '\07',
   '\010', '\011', '\012', '\013', '\014', '\015', '\016', '\017',
   '\020', '\021', '\022', '\023', '\024', '\025', '\026', '\027',
   '\030', '\031', '\032', '\033', '\034', '\035', '\036', '\037',
   ' ',    '!',    '\"',   '#',    '$',    '%',    '&',    '\'',
   '(',    ')',    '*',    '+',    ',',    '-',    '.',    '/',
   '0',    '1',    '2',    '3',    '4',    '5',    '6',    '7',
   '8',    '9',    ':',    ';',    '<',    '=',    '>',    '?',
   '@',    'A',    'B',    'C',    'D',    'E',    'F',    'G',
   'H',    'I',    'J',    'K',    'L',    'M',    'N',    'O',
   'P',    'Q',    'R',    'S',    'T',    'U',    'V',    'W',
   'X',    'Y',    'Z',    '[',    '\\',   ']',    '^',    '_',
   '`',    'A',    'B',    'C',    'D',    'E',    'F',    'G',
   'H',    'I',    'J',    'K',    'L',    'M',    'N',    'O',
   'P',    'Q',    'R',    'S',    'T',    'U',    'V',    'W',
   'X',    'Y',    'Z',    '{',    '|',    '}',    '~',    '\177',

   /* ISO-1 */

   0x80,   0x81,   0x82,   0x83,   0x84,   0x85,   0x86,   0x87,
   0x88,   0x89,   0x8a,   0x8b,   0x8c,   0x8d,   0x8e,   0x8f,
   0x90,   0x91,   0x92,   0x93,   0x94,   0x95,   0x96,   0x97,
   0x98,   0x99,   0x9a,   0x9b,   0x9c,   0x9d,   0x9e,   0x9f,
   0xa0,   0xa1,   0xa2,   0xa3,   0xa4,   0xa5,   0xa6,   0xa7,
   0xa8,   0xa9,   0xaa,   0xab,   0xac,   0xad,   0xae,   0xaf,
   0xb0,   0xb1,   0xb2,   0xb3,   0xb4,   0xb5,   0xb6,   0xb7,
   0xb8,   0xb9,   0xba,   0xbb,   0xbc,   0xbd,   0xbe,   0xbf,
   0xc0,   0xc1,   0xc2,   0xc3,   0xc4,   0xc5,   0xc6,   0xc7,
   0xc8,   0xc9,   0xca,   0xcb,   0xcc,   0xcd,   0xce,   0xcf,
   0xd0,   0xd1,   0xd2,   0xd3,   0xd4,   0xd5,   0xd6,   0xd7,
   0xd8,   0xd9,   0xda,   0xdb,   0xdc,   0xdd,   0xde,   0xdf,
   0xc0,   0xc1,   0xc2,   0xc3,   0xc4,   0xc5,   0xc6,   0xc7,
   0xc8,   0xc9,   0xca,   0xcb,   0xcc,   0xcd,   0xce,   0xcf,
   0xd0,   0xd1,   0xd2,   0xd3,   0xd4,   0xd5,   0xd6,   0xf7,
   0xd8,   0xd9,   0xda,   0xdb,   0xdc,   0xdd,   0xde,   0xff
};

const unsigned char* u__ct_tou = upper_tab + 1;
