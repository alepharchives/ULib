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
#include <ulib/internal/chttp.h>

#include <ctype.h>
#include <stdlib.h>

#ifdef HAVE_SYSEXITS_H
#  include <sysexits.h>
#else
#  include <ulib/base/replace/sysexits.h>
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
#ifndef DT_UNKNOWN
#define DT_UNKNOWN  0
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

/* Match */

int        u_pfn_flags;
bPFpcupcud u_pfn_match = u_dosmatch_with_OR;

/* Services */

int              u_num_cpu       = -1;
const char*      u_short_units[] = { "B", "KB", "MB", "GB", "TB", 0 };
struct uhttpinfo u_http_info;

#ifdef DEBUG
uint32_t u_ptr2int(void* ptr)
{
   U_INTERNAL_TRACE("u_ptr2int(%p)", ptr)

   U_INTERNAL_ASSERT_POINTER(ptr)
   U_INTERNAL_ASSERT((unsigned long)ptr <= 4294967295UL)

   return (uint32_t)((unsigned long)ptr);
}

size_t u__strlen(const char* restrict s)
{
   U_INTERNAL_TRACE("u__strlen(%s)", s)

   U_INTERNAL_ASSERT_POINTER(s)

   return strlen(s);
}

char* u_strcpy(char* restrict dest, const char* restrict src)
{
   size_t n = u__strlen(src);

   U_INTERNAL_TRACE("u_strcpy(%p,%p,%ld)", dest, src, n)

   U_INTERNAL_ASSERT_MAJOR(n,0)
   U_INTERNAL_ASSERT_POINTER(src)
   U_INTERNAL_ASSERT_POINTER(dest)
   U_INTERNAL_ASSERT(dest < src || dest > (src+n)) /* Overlapping Memory */

   (void) strcpy(dest, src);

   return dest;
}

void* u__memcpy(void* restrict dst, const void* restrict src, size_t n)
{
   U_INTERNAL_TRACE("u__memcpy(%p,%p,%ld)", dst, src, n)

   U_INTERNAL_ASSERT_MAJOR(n,0)
   U_INTERNAL_ASSERT_POINTER(src)
   U_INTERNAL_ASSERT_POINTER(dst)

   /* not overlapping memory
    *
    * non_overlap(p, s, q, t) : (p + s ≤ q) ∨ (q + t ≤ p)
    *
    * non overlap(p, s, q, t) denotes that the memory ranges p, ... , p + s − 1 and q, ... , q + t − 1 do not overlap.
    */

   U_INTERNAL_ASSERT(((((char*)src)+n) <= (char*)dst) ||
                     ((((char*)dst)+n) <= (char*)src))

   (void) memcpy(dst, src, n);

   return dst;
}

char* u_strncpy(char* restrict dest, const char* restrict src, size_t n)
{
   U_INTERNAL_TRACE("u_strncpy(%p,%p,%ld)", dest, src, n)

   U_INTERNAL_ASSERT_MAJOR(n,0)
   U_INTERNAL_ASSERT_POINTER(src)
   U_INTERNAL_ASSERT_POINTER(dest)
   U_INTERNAL_ASSERT(dest < src || dest > (src+n)) /* Overlapping Memory */

   (void) strncpy(dest, src, n);

   return dest;
}
#endif

/* Security functions */

#ifndef __MINGW32__
static uid_t real_uid      = (uid_t)(-1);
static gid_t real_gid      = (gid_t)(-1);
static uid_t effective_uid = (uid_t)(-1);
static gid_t effective_gid = (gid_t)(-1);
#endif

void u_init_security(void)
{
   /* Run this at the beginning of the program to initialize this code and
    * to drop privileges before someone uses them to shoot us in the foot
    */
#ifndef __MINGW32__
   int leffective_uid;

   U_INTERNAL_TRACE("u_init_security()")

   alarm(0); /* can be inherited from parent process */

         real_uid = getuid();
   leffective_uid = geteuid();

   /* sanity check */

   if (leffective_uid != (int) real_uid &&
       leffective_uid != 0)
      {
      U_WARNING("setuid but not to root (uid=%ld, euid=%d), dropping setuid privileges now", (long) real_uid, leffective_uid);

      u_never_need_root();
      }
   else
      {
      effective_uid = leffective_uid;
      }

   real_gid      = getgid();
   effective_gid = getegid();

   u_dont_need_root();
   u_dont_need_group();
#endif
}

/* Temporarily gain root privileges */

void u_need_root(bool necessary)
{
   U_INTERNAL_TRACE("u_need_root(%d)", necessary)

   U_INTERNAL_PRINT("(_euid_=%d, uid=%d), current=%d", effective_uid, real_uid, geteuid())

#ifndef __MINGW32__
   if (effective_uid)
      {
      if (necessary) U_ERROR(  "require root privilege but not setuid root");
                     U_WARNING("require root privilege but not setuid root");

      return;
      }

   if (real_uid == (uid_t)(-1)) U_ERROR("u_init_security() not called");

   if (geteuid() == 0) return; /* nothing to do */

   if (seteuid(effective_uid) == -1 ||
       geteuid()              !=  0)
      {
      if (necessary) U_ERROR(  "did not get root privilege");
                     U_WARNING("did not get root privilege");
      }
#endif
}

/* Temporarily drop root privileges */

void u_dont_need_root(void)
{
   U_INTERNAL_TRACE("u_dont_need_root()")

   U_INTERNAL_PRINT("(_euid_=%d, uid=%d), current=%d", effective_uid, real_uid, geteuid())

#ifndef __MINGW32__
   if (effective_uid) return;

   if (real_uid == (uid_t)(-1)) U_ERROR("u_init_security() not called");

   if (geteuid() != 0) return; /* nothing to do */

   if (seteuid(real_uid) == -1 ||
       geteuid()         != real_uid)
      {
      U_ERROR("did not drop root privilege");
      }
#endif
}

/* Permanently drop root privileges */

void u_never_need_root(void)
{
   U_INTERNAL_TRACE("u_never_need_root()")

   U_INTERNAL_PRINT("(_euid_=%d, uid=%d)", effective_uid, real_uid)

#ifndef __MINGW32__
   if (real_uid == (uid_t)(-1)) U_ERROR("u_init_security() not called");

   if (geteuid() == 0) (void) setuid(real_uid);

   if (geteuid() != real_uid ||
       getuid()  != real_uid)
      {
      U_ERROR("did not drop root privilege");
      }

    effective_uid = real_uid;
#endif
}

/* Temporarily gain group privileges */

void u_need_group(bool necessary)
{
   U_INTERNAL_TRACE("u_need_group(%d)", necessary)

   U_INTERNAL_PRINT("(egid_=%d, gid=%d)", effective_gid, real_gid)

#ifndef __MINGW32__
   if (real_gid == (gid_t)(-1)) U_ERROR("u_init_security() not called");

   if (getegid() == effective_gid) return; /* nothing to do */

    if (setegid(effective_gid) == -1 ||
        getegid()              != effective_gid)
      {
      if (necessary) U_ERROR(  "did not get group privilege");
                     U_WARNING("did not get group privilege");
      }
#endif
}

/* Temporarily drop group privileges */

void u_dont_need_group(void)
{
   U_INTERNAL_TRACE("u_dont_need_group()")

   U_INTERNAL_PRINT("(egid_=%d, gid=%d)", effective_gid, real_gid)

#ifndef __MINGW32__
   if (real_gid == (gid_t)(-1)) U_ERROR("u_init_security() not called");

   if (getegid() != effective_gid) return; /* nothing to do */

    if (setegid(real_gid) == -1 ||
        getegid()         != real_gid)
      {
      U_ERROR("did not drop group privilege");
      }
#endif
}

/* Permanently drop group privileges */

void u_never_need_group(void)
{
   U_INTERNAL_TRACE("u_never_need_group()")

   U_INTERNAL_PRINT("(egid_=%d, gid=%d)", effective_gid, real_gid)

#ifndef __MINGW32__
   if (real_gid == (gid_t)(-1)) U_ERROR("u_init_security() not called");

   if (getegid() != effective_gid) (void) setgid(real_gid);

   if (getegid() != real_gid ||
       getgid()  != real_gid)
      {
      U_ERROR("did not drop group privilege");
      }

    effective_gid = real_gid;
#endif
}

void u_setHOME(const char* restrict dir)
{
   static char buffer[128];

   U_INTERNAL_TRACE("u_setHOME(%s)", dir)

   U_INTERNAL_ASSERT_POINTER(dir)

   if (strcmp(buffer + U_CONSTANT_SIZE("HOME="), dir))
      {
      (void) snprintf(buffer, sizeof(buffer), "HOME=%s", dir);

      (void) putenv(buffer);
      }
}

/* Change the current working directory to the `user` user's home dir, and downgrade security to that user account */

bool u_runAsUser(const char* restrict user, bool change_dir)
{
#ifdef __MINGW32__
   return false;
#else
   struct passwd* restrict pw;

   U_INTERNAL_TRACE("u_runAsUser(%s,%d)", user, change_dir)

   U_INTERNAL_ASSERT_POINTER(user)

   if (!(pw = getpwnam(user)) ||
       setgid(pw->pw_gid)     ||
       setuid(pw->pw_uid))
      {
      return false;
      }

   u_setHOME(pw->pw_dir);

   (void) u_strncpy(u_user_name, user, (u_user_name_len = u__strlen(user))); /* change user name */

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

char* u_getPathRelativ(const char* restrict path, uint32_t* restrict ptr_path_len)
{
   U_INTERNAL_TRACE("u_getPathRelativ(%s,%u)", path, *ptr_path_len)

   U_INTERNAL_ASSERT_POINTER(path)
   U_INTERNAL_ASSERT_POINTER(u_cwd)

   U_INTERNAL_PRINT("u_cwd = %s", u_cwd)

   if (path[0] == u_cwd[0])
      {
      uint32_t path_len = *ptr_path_len;

      while (path[path_len-1] == '/') --path_len;

      if (path_len >= u_cwd_len &&
          memcmp(path, u_cwd, u_cwd_len) == 0)
         {
         if (path_len == u_cwd_len)
            {
            path     = ".";
            path_len = 1;
            }
         else if (     u_cwd_len  == 1 ||
                  path[u_cwd_len] == '/')
            {
            uint32_t len = u_cwd_len + (u_cwd_len > 1);

            path     += len;
            path_len -= len;

            while (path[0] == '/')
               {
               ++path;
               --path_len;
               }
            }
         }

      *ptr_path_len = path_len;

      U_INTERNAL_PRINT("path(%u) = %.*s", path_len, path_len, path)
      }

   if (path[0] == '.' &&
       path[1] == '/')
      {
      path          += 2;
      *ptr_path_len -= 2;

      U_INTERNAL_PRINT("path(%u) = %.*s", *ptr_path_len, *ptr_path_len, path)
      }

   return (char*)path;
}

/* find sequence of U_LF2 or U_CRLF2 */

uint32_t u_findEndHeader(const char* restrict str, uint32_t n)
{
   const char* restrict p;
   const char* restrict end = str + n;
   const char* restrict ptr = str;

   uint32_t pos, endHeader = U_NOT_FOUND;

   U_INTERNAL_TRACE("u_findEndHeader(%.*s,%u)", U_min(n,128), str, n)

   U_INTERNAL_ASSERT_POINTER(str)

   while (ptr < end)
      {
      p = (const char* restrict) memchr(ptr, '\n', end - ptr);

      if (p == 0) break;

      // \r\n\r\n (U_CRLF2)

      if (p[ 1] == '\r' &&
          p[-1] == '\r' &&
          p[ 2] == '\n')
         {
         pos = p - str + 3;

         if (pos <= n)
            {
            endHeader             = pos;
            u_line_terminator     = U_CRLF;
            u_line_terminator_len = 2;
            }

         break;
         }

      // \n\n (U_LF2)

      if (p[1] == '\n')
         {
         pos = p - str + 2;

         if (pos <= n)
            {
            endHeader             = pos;
            u_line_terminator     = U_LF;
            u_line_terminator_len = 1;
            }

         break;
         }

      ptr = p + 1;
      }

   return endHeader;
}

/* Determine the width of the terminal we're running on */

__pure int u_getScreenWidth(void)
{
#ifdef TIOCGWINSZ
   struct winsize wsz;
#endif

   U_INTERNAL_TRACE("u_getScreenWidth()")

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

double u_calcRate(uint64_t bytes, uint32_t msecs, int* restrict units)
{
   int i;
   double rate = (double)1000. * bytes / (double)msecs;

   U_INTERNAL_TRACE("u_calcRate(%u,%u,%p)", bytes, msecs, units)

   U_INTERNAL_ASSERT_POINTER(units)
   U_INTERNAL_ASSERT_MAJOR(bytes,0)
   U_INTERNAL_ASSERT_MAJOR(msecs,0)

   for (i = 0; rate > 1024. && u_short_units[i+1]; ++i) rate /= 1024.;

   *units = i;

   U_INTERNAL_PRINT("rate = %7.2f%s", rate, u_short_units[i])

   return rate;
}

void u_printSize(char* restrict buffer, uint64_t bytes)
{
   int units;
   double size;

   U_INTERNAL_TRACE("u_printSize(%p,%llu)", buffer, bytes)

   if (bytes == 0)
      {
      (void) u_strcpy(buffer, "0 Byte");

      return;
      }

   size = u_calcRate(bytes, 1000, &units);

   if (units) (void) sprintf(buffer, "%5.2f %s",    size, u_short_units[units]);
   else       (void) sprintf(buffer, "%7.0f Bytes", size);
}

/* get the number of the processors including offline CPUs */

static inline const char* nexttoken(const char* q, int sep)
{
   if (q) q = strchr(q, sep);
   if (q) ++q;

   return q;
}

/* When parsing bitmask lists, only allow numbers, separated by one
 * of the allowed next characters.
 *
 * The parameter 'sret' is the return from a sscanf "%u%c".  It is
 * -1 if the sscanf input string was empty.  It is 0 if the first
 * character in the sscanf input string was not a decimal number.
 * It is 1 if the unsigned number matching the "%u" was the end of the
 * input string.  It is 2 if one or more additional characters followed
 * the matched unsigned number.  If it is 2, then 'nextc' is the first
 * character following the number.  The parameter 'ok_next_chars'
 * is the nul-terminated list of allowed next characters.
 *
 * The mask term just scanned was ok if and only if either the numbers
 * matching the %u were all of the input or if the next character in
 * the input past the numbers was one of the allowed next characters.
 */

static inline bool scan_was_ok(int sret, char nextc, const char* ok_next_chars) { return (sret == 1 || (sret == 2 && strchr(ok_next_chars, nextc))); }

int u_get_num_cpu(void)
{
   U_INTERNAL_TRACE("u_get_num_cpu()")

   if (u_num_cpu == -1)
      {
#  ifdef _SC_NPROCESSORS_ONLN
      u_num_cpu = sysconf(_SC_NPROCESSORS_ONLN);
#  elif defined(_SC_NPROCESSORS_CONF)
      u_num_cpu = sysconf(_SC_NPROCESSORS_CONF);
#  else
      FILE* fp = fopen("/sys/devices/system/cpu/present", "r");

      if (fp)
         {
         char buf[128];
         const char* p;
         const char* q;

         unsigned int a;      /* begin of range */
         unsigned int b;      /* end of range */
         unsigned int s;      /* stride */
         const char *c1, *c2; /* next tokens after '-' or ',' */
         char nextc;          /* char after sscanf %u match */

         if (fgets(buf, sizeof(buf), fp))
            {
            int sret;        /* sscanf return (number of matches) */

            q = buf;

            buf[strlen(buf)-1] = '\0';

            /* Parses a comma-separated list of numbers and ranges of numbers, with optional ':%u' strides modifying ranges.
             *
             * Some examples of input lists and their equivalent simple list:
             *
             *  Input           Equivalent to
             *   0-3             0,1,2,3
             *   0-7:2           0,2,4,6
             *   1,3,5-7         1,3,5,6,7
             *   0-3:2,8-15:4    0,2,8,12
             */

            while (p = q, q = nexttoken(q, ','), p)
               {
               sret = sscanf(p, "%u%c", &a, &nextc);

               if (scan_was_ok(sret, nextc, ",-") == false) break;

               b  = a;
               s  = 1;
               c1 = nexttoken(p, '-');
               c2 = nexttoken(p, ',');

               if (c1 != 0 && (c2 == 0 || c1 < c2))
                  {
                  sret = sscanf(c1, "%u%c", &b, &nextc);

                  if (scan_was_ok(sret, nextc, ",:") == false) break;

                  c1 = nexttoken(c1, ':');

                  if (c1 != 0 && (c2 == 0 || c1 < c2))
                     {
                     sret = sscanf(c1, "%u%c", &s, &nextc);

                     if (scan_was_ok(sret, nextc, ",") == false) break;
                     }
                  }

               if (!(a <= b)) break;

               while (a <= b)
                  {
                  u_num_cpu = a + 1; /* Number of highest set bit +1 is the number of the CPUs */

                  a += s;
                  }
               }
            }

         (void) fclose(fp);
         }
#endif
      }

   return u_num_cpu;
}

/* Pin the process to a particular core */

void u_bind2cpu(pid_t pid, int n)
{
   cpu_set_t cpuset;

   U_INTERNAL_TRACE("u_bind2cpu(%d,%d)", pid, n)

   /* CPU mask of CPUs available to this process,
    * conceptually, each bit represents a logical CPU, ie:
    *
    * mask = 3  (11b):   cpu0, 1
    * mask = 13 (1101b): cpu0, 2, 3
    */

   CPU_ZERO(&cpuset);

   CPU_SET(n, &cpuset);

#ifdef HAVE_SCHED_GETAFFINITY
   (void) sched_setaffinity(pid, sizeof(cpuset), &cpuset);
#endif
}

bool u_switch_to_realtime_priority(pid_t pid)
{
   bool result = false;

#if defined(_POSIX_PRIORITY_SCHEDULING) && (_POSIX_PRIORITY_SCHEDULING > 0) && (defined(HAVE_SCHED_H) || defined(HAVE_SYS_SCHED_H))
   struct sched_param sp;

   U_INTERNAL_TRACE("u_switch_to_realtime_priority(%d)", pid)

   /* sched_getscheduler(pid); // SCHED_FIFO | SCHED_RR | SCHED_OTHER */

   (void) sched_getparam(pid, &sp);

   U_INTERNAL_PRINT("sp.sched_priority = %d", sp.sched_priority)

   sp.sched_priority = sched_get_priority_max(SCHED_FIFO);

   if (sched_setscheduler(pid, SCHED_FIFO, &sp) != -1)
      {
      U_INTERNAL_PRINT("sp.sched_priority = %d", sp.sched_priority)

      result = true;
      }
#endif

   return result;
}

__pure bool u_rmatch(const char* restrict haystack, uint32_t haystack_len, const char* restrict needle, uint32_t needle_len)
{
   U_INTERNAL_TRACE("u_rmatch(%.*s,%u,%.*s,%u)", U_min(haystack_len,128), haystack, haystack_len,
                                                 U_min(  needle_len,128),  needle,   needle_len)

   U_INTERNAL_ASSERT_POINTER(needle)
   U_INTERNAL_ASSERT_POINTER(haystack)
   U_INTERNAL_ASSERT_MAJOR(haystack_len,0)

   if (haystack_len >= needle_len)
      {
      // see if substring characters match at end

      const char* restrict nn = needle   + needle_len   - 1;
      const char* restrict hh = haystack + haystack_len - 1;

      while (*nn-- == *hh--)
         {
         if (nn >= needle) continue;

         return true; // we got all the way to the start of the substring so we must've won
         }
      }

   return false;
}

__pure void* u_find(const char* restrict s, uint32_t n, const char* restrict a, uint32_t n1)
{
   U_INTERNAL_TRACE("u_find(%.*s,%u,%.*s,%u)", U_min(n,128), s, n, U_min(n1,128), a, n1)

   U_INTERNAL_ASSERT_POINTER(s)
   U_INTERNAL_ASSERT_POINTER(a)
   U_INTERNAL_ASSERT_MAJOR(n1,0)

   if (n)
      {
      if (n1 == 1) return (void*) memchr(s, *a, n);

#ifdef HAVE_MEMMEM
      {
      void* restrict p = memmem(s, n, a, n1);

      U_INTERNAL_PRINT("memmem() = %p", p)

      return p;
      }
#else
      {
      uint32_t pos = 0;

      for (; (pos + n1) <= n; ++pos) if (memcmp(s + pos, a, n1) == 0) return (void*)(s+pos);
      }
#endif
      }

   return 0;
}

/* Search a string for any of a set of characters.
 * Locates the first occurrence in the string s of any of the characters in the string accept
 */
 
__pure const char* u_strpbrk(const char* restrict s, uint32_t slen, const char* restrict _accept)
{
   const char* restrict c;
   const char* restrict end = s + slen;

   U_INTERNAL_TRACE("u_strpbrk(%.*s,%u,%s)", U_min(slen,128), s, slen, _accept)

   U_INTERNAL_ASSERT_POINTER(s)
   U_INTERNAL_ASSERT_MAJOR(slen,0)
   U_INTERNAL_ASSERT_POINTER(_accept)

   while (s < end)
      {
      for (c = _accept; *c; ++c)
         {
         if (*s == *c) return s;
         }

      ++s;
      }

   return 0;
}

/* Search a string for a terminator of a group of delimitator {} [] () <%%>...*/

__pure const char* u_strpend(const char* restrict s, uint32_t slen,
                             const char* restrict group_delimitor, uint32_t group_delimitor_len, char skip_line_comment)
{
   char c;
   int level = 1;
   const char* restrict end = s + slen;
   uint32_t i, n = group_delimitor_len / 2;

   U_INTERNAL_TRACE("u_strpend(%.*s,%u,%s,%u,%d)", U_min(slen,128), s, slen, group_delimitor, group_delimitor_len, skip_line_comment)

   U_INTERNAL_ASSERT_POINTER(s)
   U_INTERNAL_ASSERT_MAJOR(slen,0)
   U_INTERNAL_ASSERT_POINTER(group_delimitor)
   U_INTERNAL_ASSERT_EQUALS(s[0], group_delimitor[n-1])
   U_INTERNAL_ASSERT_EQUALS(group_delimitor_len & 1, 0)

   while (s < end)
      {
loop:
      c = *++s;

      if (u_isspace(c)) continue;

      if (c == skip_line_comment)
         {
         /* skip line comment */

         s = (const char* restrict) memchr(s, '\n', end - s);

         if (s == 0) break;
         }
      else if (c == group_delimitor[0] && *(s-1) != '\\')
         {
         U_INTERNAL_PRINT("c = %c level = %d s = %.*s", c, level, 10, s)

         for (i = 1; i < n; ++i)
            {
            U_INTERNAL_PRINT("s[%d] = %c group_delimitor[%d] = %c", i, s[i], i, group_delimitor[i])

            if (s[i] != group_delimitor[i]) goto loop;
            }

         ++level;
         }
      else if (c == group_delimitor[n] && *(s-1) != '\\')
         {
         U_INTERNAL_PRINT("c = %c level = %d s = %.*s", c, level, 10, s)

         for (i = 1; i < n; ++i)
            {
            U_INTERNAL_PRINT("s[%d] = %c group_delimitor[%d] = %c", i, s[i], n+i, group_delimitor[n+i])

            if (s[i] != group_delimitor[n+i]) goto loop;
            }

         if (--level == 0) return s;
         }

      U_INTERNAL_PRINT("level = %d s = %.*s", level, 10, s)
      }

   return 0;
}

/* check if string a start with string b */

__pure bool u_startsWith(const char* restrict a, uint32_t n1, const char* restrict b, uint32_t n2)
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

__pure bool u_endsWith(const char* restrict a, uint32_t n1, const char* restrict b, uint32_t n2)
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

__pure bool u_isNumber(const char* restrict s, uint32_t n)
{
   int vdigit[]             = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0 };
   const char* restrict end = s + n;

   U_INTERNAL_TRACE("u_isNumber(%.*s,%u)", U_min(n,128), s, n)

   U_INTERNAL_ASSERT_POINTER(s)
   U_INTERNAL_ASSERT_MAJOR(n,0)

   if (*s == '+' ||
       *s == '-')
      {
      ++s;
      }

   while (s < end &&
          ((*(const unsigned char* restrict)s) >> 4) == 0x03 &&
          vdigit[(*(const unsigned char* restrict)s)  & 0x0f])
      {
      U_INTERNAL_PRINT("*s = %c, *s >> 4 = %c ", *s, (*(char* restrict)s) >> 4)

      ++s;
      }

   return (s == end);
}

/* find char not quoted */

__pure const char* u_find_char(const char* restrict s, const char* restrict end, char c)
{
   U_INTERNAL_TRACE("u_find_char(%.*s,%p,%d)", U_min(end-s,128), s, end, c)

   U_INTERNAL_ASSERT_POINTER(s)
   U_INTERNAL_ASSERT_POINTER(end)
   U_INTERNAL_ASSERT_EQUALS(s[-1],c)

   while (true)
      {
      s = (const char* restrict) memchr(s, c, end - s);

      if (s == 0) s = end;
      else
         {
         if (*(s-1) == '\\' &&
             *(s-2) != '\\')
            {
            ++s;

            continue;
            }
         }

      break;
      }

   return s;
}

/* skip string delimiter or white space and line comment */

__pure const char* u_skip(const char* restrict s, const char* restrict end, const char* restrict delim, char line_comment)
{
   U_INTERNAL_TRACE("u_skip(%.*s,%p,%s,%d)", U_min(end-s,128), s, end, delim, line_comment)

   U_INTERNAL_ASSERT_POINTER(s)
   U_INTERNAL_ASSERT_POINTER(end)

   if (delim)
      {
      // skip string delimiter

      while (s < end &&
             strchr(delim, *s))
         {
         ++s;
         }
      }
   else
      {
skipws:
      while (s < end &&
             u_isspace(*s))
         {
         ++s;
         }

      if (line_comment)
         {
         if (*s == line_comment)
            {
            // skip line comment

            s = (const char* restrict) memchr(s, '\n', end - s);

            if (s) goto skipws;

            return end;
            }
         }
      }

   return s;
}

/* delimit token */

const char* u_delimit_token(const char* restrict s, const char** restrict p, const char* restrict end, const char* restrict delim, char skip_line_comment)
{
   char c;

   U_INTERNAL_ASSERT_POINTER(s)
   U_INTERNAL_ASSERT_POINTER(p)
   U_INTERNAL_ASSERT_POINTER(end)

   U_INTERNAL_TRACE("u_delimit_token(%.*s,%p,%p,%s,%d)", U_min(end-s,128), s, p, end, delim, skip_line_comment)

   s = u_skip(s, end, delim, skip_line_comment);

   U_INTERNAL_PRINT("s = %p end = %p", s, end)

   if (s == end) return ++end;

   *p = s++;
    c = **p;

   if (c == '"'  ||
       c == '\'')
      {
      s = u_find_char(s, end, c);

      if (delim)
         {
         if (s < end) ++s;
         }
      else
         {
         ++(*p);
         }
      }
   else if (delim)
      {
      s = (const char* restrict) u_strpbrk(s, end - s, delim);

      if (s == 0) return end;
      }
   else
      {
      // find next white space

      while (s < end &&
             u_isspace(*s) == false)
         {
         ++s;
         }
      }

   return s;
}

uint32_t u_split(char* restrict s, uint32_t n, char** restrict argv, const char* restrict delim)
{
   const char* restrict p;
   char* restrict end  = s + n;
   char** restrict ptr = argv;

   U_INTERNAL_TRACE("u_split(%.*s,%u,%p,%s)", U_min(n,128), s, n, argv, delim)

   U_INTERNAL_ASSERT_POINTER(s)
   U_INTERNAL_ASSERT_MAJOR(n,0)
   U_INTERNAL_ASSERT_POINTER(argv)
   U_INTERNAL_ASSERT_EQUALS(u_isBinary((const unsigned char*)s,n),false)

   while (s < end)
      {
      s = (char* restrict) u_delimit_token(s, (const char** restrict)&p, end, delim, 0);

      U_INTERNAL_PRINT("s = %.*s", 20, s)

      if (s <= end)
         {
         *argv++ = (char* restrict) p;

         *s++ = '\0';

         U_INTERNAL_PRINT("u_split() = %s", p)
         }
      }

   *argv = 0;

   n = (argv - ptr);

   return n;
}

/* Match STRING against the filename pattern MASK, returning true if it matches, false if not, inversion if flags contain FNM_INVERT */

__pure bool u_dosmatch(const char* restrict s, uint32_t n1, const char* restrict mask, uint32_t n2, int flags)
{
   bool result;
   const char* restrict cp = 0;
   const char* restrict mp = 0;
   unsigned char c1 = 0, c2 = 0;

   const char* restrict end_s    =    s + n1;
   const char* restrict end_mask = mask + n2;

   U_INTERNAL_TRACE("u_dosmatch(%.*s,%u,%.*s,%u,%d)", U_min(n1,128), s, n1, n2, mask, n2, flags)

   U_INTERNAL_ASSERT_POINTER(s)
   U_INTERNAL_ASSERT_POINTER(mask)
   U_INTERNAL_ASSERT_MAJOR(n1,0)
   U_INTERNAL_ASSERT_MAJOR(n2,0)

   if (flags & FNM_IGNORECASE)
      {
      while (s < end_s)
         {
         c2 = u_tolower(*mask);

         if (c2 == '*') break;

         c1 = u_tolower(*s);

         if (c2 != c1 &&
             c2 != '?') return (flags & FNM_INVERT ? true : false);

         ++s;
         ++mask;
         }

      U_INTERNAL_PRINT("c1 = %c c2 = %c", c1, c2)

      while (true)
         {
         if (s >= end_s)
            {
            while (*mask == '*') ++mask;

            result = (mask >= end_mask);

            return (flags & FNM_INVERT ? (result != true) : result);
            }

         c2 = u_tolower(*mask);

         if (c2 == '*')
            {
            if (++mask >= end_mask) return (flags & FNM_INVERT ? false : true);

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
             c2 != '?') return (flags & FNM_INVERT ? true : false);

         ++s;
         ++mask;
         }

      U_INTERNAL_PRINT("c1 = %c c2 = %c", c1, c2)

      while (true)
         {
         if (s >= end_s)
            {
            while (*mask == '*') ++mask;

            result = (mask >= end_mask);

            return (flags & FNM_INVERT ? (result != true) : result);
            }

         c2 = *mask;

         if (c2 == '*')
            {
            if (++mask >= end_mask) return (flags & FNM_INVERT ? false : true);

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

/* Match STRING against the filename pattern MASK and multiple patterns separated by '|',
   returning true if it matches, false if not, inversion if flags contain FNM_INVERT
*/

__pure bool u_dosmatch_with_OR(const char* restrict s, uint32_t n1, const char* restrict mask, uint32_t n2, int flags)
{
   bool result;
   const char* restrict p_or;
   const char* restrict end = mask + n2;

   U_INTERNAL_TRACE("u_dosmatch_with_OR(%.*s,%u,%.*s,%u,%d)", U_min(n1,128), s, n1, n2, mask, n2, flags)

   U_INTERNAL_ASSERT_POINTER(s)
   U_INTERNAL_ASSERT_POINTER(mask)
   U_INTERNAL_ASSERT_MAJOR(n1,0)
   U_INTERNAL_ASSERT_MAJOR(n2,0)

   while (true)
      {
      p_or = (const char* restrict) memchr(mask, '|', n2);

      if (p_or == 0)
         {
         result = u_dosmatch(s, n1, mask, n2, flags);

         U_INTERNAL_PRINT("result = %d", result)

         return result;
         }

      result = u_dosmatch(s, n1, mask, (p_or - mask), flags);

      U_INTERNAL_PRINT("result = %d", result)

      if (flags & FNM_INVERT)
         {
         if (result == false) return false;
         }
      else
         {
         if (result) return true;
         }

      mask = p_or + 1;
      n2   = end - mask;
      }
}

__pure bool u_isHostName(const char* restrict ptr, uint32_t len)
{
   int ch;
   const char* restrict end = ptr + len;

   U_INTERNAL_TRACE("u_isHostName(%.*s,%u)", U_min(len,128), ptr, len)

   U_INTERNAL_ASSERT_POINTER(ptr)

   if (ptr[ 0] == '[' &&
       end[-1] == ']')
      {
      if (u_isIPv6Addr(ptr+1, len-1)) return true;

      return false;
      }

   ch = *(unsigned char*)ptr;

   /* Host names may contain only alphanumeric characters, minus signs ("-"), and periods (".").
    * They must begin with an alphabetic character and end with an alphanumeric character
    *
    * Several well known Internet and technology companies have DNS records that use the underscore:
    * http://domainkeys.sourceforge.net/underscore.html
    */

   if (u_isalpha(ch))
      {
      if (u_isalnum(end[-1]) == false) return false;

      while (++ptr < end)
         {
         ch = *(unsigned char*)ptr;

         if (u_ishname(ch) == false) return false;
         }

      return true;
      }

   while (ptr < end)
      {
      ch = *(unsigned char*)ptr;

      if ((u_isdigit( ch) == 0 && ch != '.') &&
          (u_isxdigit(ch) == 0 && ch != '.'  && ch != ':'))
         {
         return false;
         }

      ++ptr;
      }

   return true;
}

__pure const char* u_isUrlScheme(const char* restrict url, uint32_t len)
{
   int ch;
   const char* restrict ptr;
   bool alphanumeric, special;
   const char* restrict first_slash;

   U_INTERNAL_TRACE("u_isUrlScheme(%.*s,%u)", U_min(len,128), url, len)

   U_INTERNAL_ASSERT_POINTER(url)

   first_slash = (const char* restrict) memchr(url, '/', len);

   if (first_slash     == 0   ||
       first_slash     == url ||     /* Input with no slash at all or slash first can't be URL */
       first_slash[-1] != ':' ||
       first_slash[ 1] != '/' ||     /* Character before must be : and next must be / */
       first_slash     == (url + 1)) /* There must be something before the :// */
      {
      return 0;
      }

   /* Check all characters up to first slash - 1. Only alphanum is allowed */

   ptr = url;

   --first_slash;

   U_INTERNAL_ASSERT_EQUALS(first_slash[0], ':')

   while (ptr < first_slash)
      {
      ch = *(unsigned char*)ptr;

      /* The set of valid URL schemes, as per STD66 (RFC3986) is '[A-Za-z][A-Za-z0-9+.-]*'.
       * But use sightly looser check of '[A-Za-z0-9][A-Za-z0-9+.-]*' because earlier version
       * of check used '[A-Za-z0-9]+' so not to break any remote helpers
       */

      special      = (ch == '+' || ch == '-' || ch == '.');
      alphanumeric = (ch > 0 && u_isalnum(ch));

      if (alphanumeric            == false &&
          (ptr != url && special) == false)
         {
         return 0;
         }

      ++ptr;
      }

   U_INTERNAL_ASSERT_EQUALS(ptr[1], '/')
   U_INTERNAL_ASSERT_EQUALS(ptr[2], '/')

   return ptr+3;
}

__pure bool u_isURL(const char* restrict url, uint32_t len)
{
   const char* restrict ptr;

   U_INTERNAL_TRACE("u_isURL(%.*s,%u)", U_min(len,128), url, len)

   /* proto://hostname[:port]/[path]?[query] */

   ptr = (const char* restrict) u_isUrlScheme(url, len);

   if (ptr)
      {
      int ch;
      const char* restrict end;
      const char* restrict tmp;

      len -= (ptr - url);

      tmp  = ptr;
      end  = ptr + len;

      while (tmp < end)
         {
         ch = *(unsigned char*)tmp;

         if (ch == '/' ||
             ch == '?')
            {
            len = (end = tmp) - ptr;

            break;
            }

         ++tmp;
         }

      U_INTERNAL_PRINT("ptr = %.*s", U_min(len,128), ptr)

      tmp = (const char* restrict) memrchr(ptr, ':', len);

      if (        tmp &&
          ((end - tmp) <= 5)) /* NB: port number: 0-65536 */
         {
         len = tmp - ptr;

         while (++tmp < end)
            {
            ch = *(unsigned char*)tmp;

            U_INTERNAL_PRINT("ch = %c", ch)

            if (u_isdigit(ch) == 0) return false;
            }
         }

      if (u_isHostName(ptr, len)) return true;
      }

   return false;
}

__pure bool u_isMacAddr(const char* restrict p, uint32_t len)
{
   uint32_t c;

   U_INTERNAL_TRACE("u_isMacAddr(%.*s,%u)", U_min(len,128), p, len)

   /* cisco-style: 0123.4567.89ab */

   if (len == (3 * 5) - 1)
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

   if (len == (6 * 3) - 1)
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

__pure bool u_validate_email_address(const char* restrict address, uint32_t address_len)
{
   int count;
   const char* restrict c;
   const char* restrict end;
   const char* restrict domain;

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

__pure int u_strnatcmp(char const* restrict a, char const* restrict b)
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

#ifdef __MINGW32__
#  define PATH_LIST_SEP ';'
#else
#  define PATH_LIST_SEP ':'
#endif

/* Given a string containing units of information separated by colons, return the next one pointed to by (P_INDEX), or NULL if there are no more.
 * Advance (P_INDEX) to the character after the colon
 */

static inline char* extract_colon_unit(char* restrict pzDir, const char* restrict string, uint32_t string_len, uint32_t* restrict p_index)
{
         char* restrict pzDest = pzDir;
   const char* restrict pzSrc  = string + *p_index;

   U_INTERNAL_TRACE("extract_colon_unit(%s,%.*s,%u,%p)", pzDir, string_len, string, string_len, p_index)

   if ((string == 0) ||
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

static inline void make_absolute(char* restrict result, const char* restrict dot_path, const char* restrict string)
{
   int result_len;

   U_INTERNAL_TRACE("make_absolute(%p,%s,%s)", result, dot_path, string)

   U_INTERNAL_ASSERT_POINTER(dot_path)

   if (dot_path[0])
      {
      u_strcpy(result, dot_path);

      result_len = u__strlen(result);

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

   u_strcpy(result + result_len, string);
}

/* --------------------------------------------------------------------------------------------------------------------------------------------------------
 * find a FILE MODE along PATH
 * --------------------------------------------------------------------------------------------------------------------------------------------------------
 * pathfind looks for a a file with name FILENAME and MODE access along colon delimited PATH, and build the full pathname as a string, or NULL if not found
 * --------------------------------------------------------------------------------------------------------------------------------------------------------
 */

#ifdef __MINGW32__
#  define U_PATH_DEFAULT "C:\\msys\\1.0\\bin;C:\\MinGW\\bin;C:\\windows;C:\\windows\\system;C:\\windows\\system32"

static const char* u_check_for_suffix_exe(const char* restrict program)
{
   static char program_w32[MAX_FILENAME_LEN + 1];

   int len = u__strlen(program);

   U_INTERNAL_TRACE("u_check_for_suffix_exe(%s)", program)

   if (u_endsWith(program, len, U_CONSTANT_TO_PARAM(".exe")) == false)
      {
      (void) u__memcpy(program_w32, program, len);
      (void) u__memcpy(program_w32+len, ".exe", sizeof(".exe"));

      program = program_w32;

      U_INTERNAL_PRINT("program = %s", program)
      }

   return program;
}
#else
#  define U_PATH_DEFAULT "/sbin:/usr/sbin:/usr/local/sbin:/bin:/usr/bin:/usr/local/bin"
#endif

bool u_pathfind(char* restrict result, const char* restrict path, uint32_t path_len, const char* restrict filename, int mode)
{
   uint32_t p_index = 0;
   char* restrict colon_unit;
   char zPath[U_PATH_MAX + 1];

   U_INTERNAL_TRACE("u_pathfind(%p,%.*s,%u,%s,%d)", result, path_len, path, path_len, filename, mode)

   if (path_len == 0)
      {
      path = getenv("PATH");

      if (path) path_len = u__strlen(path);
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

      if (colon_unit == 0) break;

      make_absolute(result, colon_unit, filename);

      /* Make sure we can access it in the way we want */

      if (access(result, mode) >= 0)
         {
         /* We can, so normalize the name and return it below */

         (void) u_canonicalize_pathname(result);

         return true;
         }
      }

   return false;
}

/* ----------------------------------------------------------------------------------
 * Canonicalize PATH, and build a new path. The new path differs from PATH in that:
 * ----------------------------------------------------------------------------------
 * Multiple    '/'                     are collapsed to a single '/'
 * Leading     './'  and trailing '/.' are removed
 * Trailing    '/'                     are removed
 * Trailing    '/.'                    are removed
 * Non-leading '../' and trailing '..' are handled by removing portions of the path
 * ----------------------------------------------------------------------------------
 */

bool u_canonicalize_pathname(char* restrict path)
{
   int len;
   char* restrict p;
   char* restrict s;
   char* restrict src;
   char* restrict dst;
   bool is_modified = false;
   char* restrict lpath = path;

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

         while (*(++s) == '/') {}

         is_modified = true;

         for (src = s, dst = p + 1; (*dst = *src); ++src, ++dst) {} /* u_strcpy(p + 1, s); */

         U_INTERNAL_PRINT("path = %s", path)
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
         is_modified = true;

         for (src = p + 2, dst = p; (*dst = *src); ++src, ++dst) {} /* u_strcpy(p, p + 2); */

         U_INTERNAL_PRINT("path = %s", path)
         }
      else
         {
         ++p;
         }
      }

   /* Remove trailing slashes */

   p = lpath + u__strlen(lpath) - 1;

   if ( p > lpath &&
       *p == '/')
      {
      is_modified = true;

      do { *p-- = '\0'; } while (p > lpath && *p == '/');
      }

   /* Remove leading "./" */

   if (lpath[0] == '.' &&
       lpath[1] == '/')
      {
      if (lpath[2] == 0)
         {
         lpath[1] = 0;

         return true;
         }

      is_modified = true;

      for (src = lpath + 2, dst = lpath; (*dst = *src); ++src, ++dst) {} /* u_strcpy(lpath, lpath + 2); */

      U_INTERNAL_PRINT("path = %s", path)
      }

   /* Remove trailing "/" or "/." */

   len = u__strlen(lpath);

   if (len < 2) goto end;

   if (lpath[len - 1] == '/')
      {
      lpath[len - 1] = 0;

      is_modified = true;
      }
   else
      {
      if (lpath[len - 1] == '.' &&
          lpath[len - 2] == '/')
         {
         if (len == 2)
            {
            lpath[1] = 0;

            return true;
            }

         is_modified = true;

         lpath[len - 2] = 0;
         }
      }

   /* Collapse "/.." with the previous part of path */

   p = lpath;

   while (p[0] &&
          p[1] &&
          p[2])
      {
      if ((p[0] != '/'  ||
           p[1] != '.'  ||
           p[2] != '.') ||
          (p[3] != '/'  &&
           p[3] != 0))
         {
         ++p;

         continue;
         }

      /* search for the previous token */

      s = p - 1;

      while (s >= lpath && *s != '/') --s;

      ++s;

      /* If the previous token is "..", we cannot collapse it */

      if (s[0]    == '.' &&
          s[1]    == '.' &&
          (s + 2) == p)
         {
         p += 3;

         continue;
         }

      if (p[3] != '\0')
         {
         /*      "/../foo" -> "/foo" */
         /* "token/../foo" ->  "foo" */

         is_modified = true;

         for (src = p + 4, dst = s + (s == lpath && *s == '/'); (*dst = *src); ++src, ++dst) {} /* u_strcpy(s + (s == lpath && *s == '/'), p + 4); */

         U_INTERNAL_PRINT("path = %s", path)

         p = s - (s > lpath);

         continue;
         }

      /* trailing ".." */

      is_modified = true;

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
         }

      break;
      }

end:
   U_INTERNAL_PRINT("path = %s", path)

   return is_modified;
}

/* Prepare command for call to exec() */

int u_splitCommand(char* restrict s, uint32_t n, char** restrict argv, char* restrict pathbuf, uint32_t pathbuf_size)
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
      argv[1] = (char* restrict) u_basename(argv[0]);

      pathbuf[0] = '\0';
      }
   else
      {
      argv[0] = pathbuf;

#  ifdef __MINGW32__
      argv[1] = (char* restrict) u_check_for_suffix_exe(argv[1]);
#  endif

      if (u_pathfind(pathbuf, 0, 0, argv[1], R_OK | X_OK) == false) return -1;

      U_INTERNAL_ASSERT_MINOR(u__strlen(pathbuf), pathbuf_size)
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
   char* restrict free;
   struct u_dirent_s* dp;
   uint32_t num, max, pfree, nfree, szfree;
};

struct u_ftw_ctx_s u_ftw_ctx;

#define U_DIRENT_ALLOCATE   (128U * 1024U)
#define U_FILENAME_ALLOCATE ( 32U * U_DIRENT_ALLOCATE)

static inline void u_ftw_allocate(struct u_dir_s* restrict u_dir)
{
   U_INTERNAL_TRACE("u_ftw_allocate(%p)", u_dir)

   if (u_ftw_ctx.sort_by)
      {
      u_dir->free  = (char* restrict)     malloc((u_dir->nfree = u_dir->szfree = U_FILENAME_ALLOCATE));
      u_dir->dp    = (struct u_dirent_s*) malloc((u_dir->max                   = U_DIRENT_ALLOCATE) * sizeof(struct u_dirent_s));
      u_dir->pfree = 0U;
      }
   else
      {
      (void) memset(u_dir, 0, sizeof(struct u_dir_s));
      }

   u_dir->num = 0U;
}

static inline void u_ftw_deallocate(struct u_dir_s* restrict u_dir)
{
   U_INTERNAL_TRACE("u_ftw_deallocate(%p)", u_dir)

   if (u_ftw_ctx.sort_by)
      {
      free(u_dir->free);
      free(u_dir->dp);
      }
}

static inline void u_ftw_reallocate(struct u_dir_s* restrict u_dir, uint32_t d_namlen)
{
   U_INTERNAL_TRACE("u_ftw_reallocate(%p,%u)", u_dir, d_namlen)

   U_INTERNAL_ASSERT_DIFFERS(u_ftw_ctx.sort_by,0)

   if (u_dir->num >= u_dir->max)
      {
      u_dir->max += U_DIRENT_ALLOCATE;

      U_INTERNAL_PRINT("Reallocating u_dir->dp to size %u", u_dir->max)

      u_dir->dp = (struct u_dirent_s* restrict) realloc(u_dir->dp, u_dir->max * sizeof(struct u_dirent_s));

      U_INTERNAL_ASSERT_POINTER(u_dir->dp)
      }

   if (d_namlen > u_dir->nfree)
      {
      u_dir->nfree  += U_FILENAME_ALLOCATE;
      u_dir->szfree += U_FILENAME_ALLOCATE;

      U_INTERNAL_PRINT("Reallocating u_dir->free to size %u", u_dir->szfree)

      u_dir->free = (char* restrict) realloc(u_dir->free, u_dir->szfree);

      U_INTERNAL_ASSERT_POINTER(u_dir->free)
      }
}

static void u_ftw_call(char* restrict d_name, uint32_t d_namlen, unsigned char d_type)
{
   U_INTERNAL_TRACE("u_ftw_call(%.*s,%u,%d)", d_namlen, d_name, d_namlen, d_type)

   if (d_type != DT_REG &&
       d_type != DT_DIR &&
       d_type != DT_LNK &&
       d_type != DT_UNKNOWN) return;

   (void) u__memcpy(u_buffer + u_buffer_len, d_name, d_namlen);

   u_buffer_len += d_namlen;

   U_INTERNAL_ASSERT_MINOR(u_buffer_len,4096)

   u_buffer[u_buffer_len] = '\0';

   u_ftw_ctx.is_directory = (d_type == DT_DIR);

   if ( u_ftw_ctx.depth &&
       (u_ftw_ctx.is_directory ||
        d_type == DT_LNK       ||
        d_type == DT_UNKNOWN))
      {
      u_ftw();

      goto end;
      }

   u_ftw_ctx.call();

end:
   u_buffer_len -= d_namlen;
}

__pure int u_ftw_ino_cmp(const void* restrict a, const void* restrict b)
{ return (((const struct u_dirent_s* restrict)a)->d_ino - ((const struct u_dirent_s* restrict)b)->d_ino); }

static void u_ftw_readdir(DIR* restrict dirp)
{
   uint32_t i, d_namlen;
   struct u_dir_s u_dir;
   struct dirent* restrict dp;
   struct u_dirent_s* restrict ds;

   U_INTERNAL_TRACE("u_ftw_readdir(%p)", dirp)

   u_ftw_allocate(&u_dir);

   /* -----------------------------------------
    * NB: NON sono sempre le prime due entry !!
    * -----------------------------------------
    * (void) readdir(dirp);         // skip '.'
    * (void) readdir(dirp);         // skip '..'
    * -----------------------------------------
    */

   while ((dp = (struct dirent* restrict) readdir(dirp)))
      {
      d_namlen = NAMLEN(dp);

      U_INTERNAL_PRINT("d_namlen = %u d_name = %.*s", d_namlen, d_namlen, dp->d_name)

      if (U_ISDOTS(dp->d_name)) continue;

      if (u_ftw_ctx.filter_len == 0 ||
          u_pfn_match(dp->d_name, d_namlen, u_ftw_ctx.filter, u_ftw_ctx.filter_len, u_pfn_flags))
         {
         if (u_ftw_ctx.sort_by)
            {
            u_ftw_reallocate(&u_dir, d_namlen); /* check se necessarie nuove allocazioni */

            ds = u_dir.dp + u_dir.num++;

            ds->d_ino  = dp->d_ino;
            ds->d_type = U_DT_TYPE;

            (void) u__memcpy(u_dir.free + (ds->d_name = u_dir.pfree), dp->d_name, (ds->d_namlen = d_namlen));

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
   DIR* restrict dirp;

   U_INTERNAL_TRACE("u_ftw()")

   U_INTERNAL_ASSERT_EQUALS(u__strlen(u_buffer), u_buffer_len)

   /*
    * NB: if is present the char 'filetype' this item isn't a directory and we don't need to try opendir()...
    *
    * dirp = (DIR*) (u_ftw_ctx.filetype && strchr(u_buffer+1, u_ftw_ctx.filetype) ? 0 : opendir(u_buffer));
    */

   u_ftw_ctx.is_directory = ((dirp = (DIR* restrict) opendir(U_PATH_CONV(u_buffer))) != 0);

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
   static int _random = 1;

   U_INTERNAL_TRACE("u_get_num_random(%d)", range)

   U_INTERNAL_ASSERT_MAJOR(range,0)

   _random = (_random * 1103515245 + 12345) & 0x7fffffff;

   return ((_random % range) + 1);
}

#ifdef USE_LIBSSL
int u_passwd_cb(char* restrict buf, int size, int rwflag, void* restrict password)
{
   U_INTERNAL_TRACE("u_passwd_cb(%p,%d,%d,%p)", buf, size, rwflag, password)

   (void) u__memcpy(buf, (char* restrict)password, size);

   buf[size-1] = '\0';

   size = u__strlen(buf);

   U_INTERNAL_PRINT("buf(%d) = %.*s", size, U_min(size,128), buf)

   return size;
}
#endif

/*
 * Function fnmatch() as specified in POSIX 1003.2-1992, section B.6.
 * Compares a filename or pathname to a pattern.
 */

static const char* restrict end_p;
static const char* restrict end_s;

static inline int rangematch(const char* restrict pattern, char test, int flags, char** restrict newp)
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

      if (c == '/' && (flags & FNM_PATHNAME)) return 0;

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

   *newp = (char* restrict) pattern;

   return (ok == negate ? 0 : 1);
}

static __pure int kfnmatch(const char* restrict pattern, const char* restrict string, int flags, int nesting)
{
   char c, test;
   char* restrict newp;
   const char* restrict stringstart;

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
                  return ((flags & FNM_LEADING_DIR) || memchr(string, '/', end_s - string) == 0 ? 0 : 1);
                  }
               else
                  {
                  return (0);
                  }
               }
            else if (c == '/' && flags & FNM_PATHNAME)
               {
               if ((string = (const char* restrict)memchr(string, '/', end_s - string)) == 0) return (1);

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

            switch (rangematch(pattern, (char)*string,  flags, (char** restrict)&newp))
               {
               case -1: goto norm;
               case  1: pattern = newp; break;
               case  0: return 1;
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

#define __FNM_FLAGS (FNM_PATHNAME | FNM_NOESCAPE | FNM_PERIOD | FNM_LEADING_DIR | FNM_CASEFOLD | FNM_INVERT)

bool u_fnmatch(const char* restrict string, uint32_t n1, const char* restrict pattern, uint32_t n2, int flags)
{
   int result;

   U_INTERNAL_TRACE("u_fnmatch(%.*s,%u,%.*s,%u,%d)", U_min(n1,128), string, n1, n2, pattern, n2, flags)

   U_INTERNAL_ASSERT_MAJOR(n1,0)
   U_INTERNAL_ASSERT_MAJOR(n2,0)
   U_INTERNAL_ASSERT_POINTER(string)
   U_INTERNAL_ASSERT_POINTER(pattern)
   U_INTERNAL_ASSERT_EQUALS((flags & ~__FNM_FLAGS),0)

   end_s = string  + n1;
   end_p = pattern + n2;

   result = kfnmatch(pattern, string, flags, 0);

   return (flags & FNM_INVERT ? (result != 0) : (result == 0));
}

#define U_LOOP_STRING( exec_code ) unsigned char c = *s; while (n--) { exec_code ; c = *(++s); }

/*
#if !defined(GCOV)

* (Duff's device) This is INCREDIBLY ugly, but fast. We break the string up into 8 byte units. On the first
* time through the loop we get the "leftover bytes" (strlen % 8). On every other iteration, we perform 8 BODY's
* so we handle all 8 bytes. Essentially, this saves us 7 cmp & branch instructions. If this routine is heavily
* used enough, it's worth the ugly coding

#  undef  U_LOOP_STRING
#  define U_LOOP_STRING( exec_code ) {     \
   unsigned char c;                        \
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
*/

__pure bool u_isName(const char* restrict s, uint32_t n)
{
   U_LOOP_STRING( if (u_isname(c) == false) return false )

   U_INTERNAL_TRACE("u_isName(%.*s,%u)", U_min(n,128), s, n)

   return true;
}

__pure bool u_isBase64(const char* restrict s, uint32_t n)
{
   U_LOOP_STRING( if (u_isbase64(c) == false) return false )
 
   U_INTERNAL_TRACE("u_isBase64(%.*s,%u)", U_min(n,128), s, n)

   return true;
}

__pure bool u_isWhiteSpace(const char* restrict s, uint32_t n)
{
   U_LOOP_STRING( if (u_isspace(c) == false) return false )

   U_INTERNAL_TRACE("u_isWhiteSpace(%.*s,%u)", U_min(n,128), s, n)

   return true;
}

__pure bool u_isText(const unsigned char* restrict s, uint32_t n)
{
   U_LOOP_STRING( if (u_istext(c) == false) return false )

   U_INTERNAL_TRACE("u_isText(%.*s,%u)", U_min(n,128), s, n)

   return true;
}

__pure bool u_isBinary(const unsigned char* restrict s, uint32_t n) { return ((u_isText(s,n) || u_isUTF8(s,n) || u_isUTF16(s,n)) == false); }

/* From RFC 3986

#define U_URI_UNRESERVED  0 // ALPHA (%41-%5A and %61-%7A) DIGIT (%30-%39) '-' '.' '_' '~'
#define U_URI_PCT_ENCODED 1
#define U_URI_GEN_DELIMS  2 // ':' '/' '?' '#' '[' ']' '@'
#define U_URI_SUB_DELIMS  4 // '!' '$' '&' '\'' '(' ')' '*' '+' ',' ';' '='
*/

unsigned u_uri_encoded_char_mask = (U_URI_PCT_ENCODED | U_URI_GEN_DELIMS | U_URI_SUB_DELIMS);

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

/************************************************************************
 * From rfc2044: encoding of the Unicode values on UTF-8:               *
 *                                                                      *
 * UCS-4 range (hex.)           UTF-8 octet sequence (binary)           *
 * 0000 0000-0000 007F   0xxxxxxx                                       *
 * 0000 0080-0000 07FF   110xxxxx 10xxxxxx                              *
 * 0000 0800-0000 FFFF   1110xxxx 10xxxxxx 10xxxxxx                     *
 ************************************************************************/

__pure bool u_isUTF8(const unsigned char* restrict buf, uint32_t len)
{
   unsigned char c;
   bool result = false;
   uint32_t j, following;
   const unsigned char* restrict end = buf + len;

   U_INTERNAL_TRACE("u_isUTF8(%.*s,%u)", U_min(len,128), buf, len)

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
         if ((u_cttab(c) & 0x0200) == 0) return false;
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
            if (buf >= end) return result;

            c = *buf++;

            if ((c & 0x80) == 0 ||
                (c & 0x40))
               {
               return false;
               }
            }

         result = true;
         }
      }

   return result;
}

__pure int u_isUTF16(const unsigned char* restrict buf, uint32_t len)
{
   uint32_t be, i, c;

   U_INTERNAL_TRACE("u_isUTF16(%.*s,%u)", U_min(len,128), buf, len)

   if (len < 2) return 0;

   if      (buf[0] == 0xff && buf[1] == 0xfe) be = 0;
   else if (buf[0] == 0xfe && buf[1] == 0xff) be = 1;
   else                                       return 0;

   for(i = 2; i + 1 < len; i += 2)
      {
      c = (be ? buf[i+1] + 256 * buf[i]
              : buf[i]   + 256 * buf[i+1]);

      if ( c == 0xfffe ||
          (c  < 128 && ((u_cttab(c) & 0x0200) == 0)))
         {
         return 0;
         }
      }

   return (1 + be);
}

/* Table for character type identification
 * Shorter definitions to make the data more compact
 */

#define C 0x0001 /* Control character */
#define D 0x0002 /* Digit */
#define L 0x0004 /* Lowercase */
#define P 0x0008 /* Punctuation */
#define S 0x0010 /* Space */
#define U 0x0020 /* Uppercase */
#define X 0x0040 /* Hexadecimal */
#define B 0x0080 /* Blank (a space or a tab) */
#define R 0x0100 /* carriage return or new line (a \r or \n) */
#define T 0x0200 /* character appears in plain ASCII text */
#define F 0x0400 /* character never appears in text */
#define O 0x0800 /* character minus    '-' (45 0x2D) */
#define N 0x1000 /* character point    '.' (46 0x2E) */
#define M 0x2000 /* character underbar '_' (95 0x5F) */

#define CS   (C | S)
#define CT   (C | T)
#define CF   (C | F)
#define DT   (D | T)
#define UT   (U | T)
#define LU   (L | U)
#define LX   (L | X)
#define LT   (L | T)
#define UX   (U | X)
#define SB   (S | B)
#define PT   (P | T)
#define PTO  (P | T | O)
#define PTN  (P | T | N)
#define PTM  (P | T | M)
#define LXT  (L | X | T)
#define UXT  (U | X | T)
#define SBT  (S | B | T)
#define CST  (C | S | T)
#define CSB  (C | S | B)
#define CSF  (C | S | F)
#define CSR  (C | S | R)
#define CSBT (C | S | B | T)
#define CSRT (C | S | R | T)

const unsigned short u__ct_tab[256] = {
   /*                                BEL  BS    HT    LF        FF    CR       */
   CF,  CF,  CF,  CF,  CF,  CF,  CF,  CT, CT, CSBT, CSRT, CSF, CST, CSRT, CF,  CF, /* 0x0X */
   /*                                                     ESC                  */
   CF,  CF,  CF,  CF,  CF,  CF,  CF,  CF, CF,   CF,   CF,  CT,  CF,  CF,  CF,  CF, /* 0x1X */

   SBT, PT,  PT,  PT,  PT,  PT,  PT,  PT,  PT,  PT,   PT,  PT,  PT,  PTO, PTN, PT, /* 0x2X */
   DT,  DT,  DT,  DT,  DT,  DT,  DT,  DT,  DT,  DT,   PT,  PT,  PT,  PT,  PT,  PT, /* 0x3X */
   PT,  UXT, UXT, UXT, UXT, UXT, UXT, UT,  UT,  UT,   UT,  UT,  UT,  UT,  UT,  UT, /* 0x4X */
   UT,  UT,  UT,  UT,  UT,  UT,  UT,  UT,  UT,  UT,   UT,  PT,  PT,  PT,  PT,  PTM,/* 0x5X */
   PT, LXT, LXT, LXT, LXT, LXT, LXT,  LT,  LT,  LT,   LT,  LT,  LT,  LT,  LT,  LT, /* 0x6X */
   LT,  LT,  LT,  LT,  LT,  LT,  LT,  LT,  LT,  LT,   LT,  PT,  PT,  PT,  PT,  CF, /* 0x7X */

   /* Assume an ISO-1 character set */

   /*                NEL                                             */
   C,  C,  C,  C,  C, CT,  C,  C,  C,    C,   C,   C,   C,  C,   C,  C, /* 0x8X */
   C,  C,  C,  C,  C,  C,  C,  C,  C,    C,   C,   C,   C,  C,   C,  C, /* 0x9X */
   S,  P,  P,  P,  P,  P,  P,  P,  P,    P,   P,   P,   P,  P,   P,  P, /* 0xaX */
   P,  P,  P,  P,  P,  P,  P,  P,  P,    P,   P,   P,   P,  P,   P,  P, /* 0xbX */
   U,  U,  U,  U,  U,  U,  U,  U,  U,    U,   U,   U,   U,  U,   U,  U, /* 0xcX */
   U,  U,  U,  U,  U,  U,  U,  P,  U,    U,   U,   U,   U,  U,   U, LU, /* 0xdX */
   L,  L,  L,  L,  L,  L,  L,  L,  L,    L,   L,   L,   L,  L,   L,  L, /* 0xeX */
   L,  L,  L,  L,  L,  L,  L,  P,  L,    L,   L,   L,   L,  L,   L,  L  /* 0xfX */
};

#undef C
#undef D
#undef L
#undef P
#undef S
#undef U
#undef X
#undef B
#undef R
#undef T
#undef F
#undef O
#undef N
#undef M

#undef CS
#undef CT
#undef CF
#undef LU
#undef LX
#undef UX
#undef SB
#undef DT
#undef UT
#undef LT
#undef PT
#undef PTO
#undef PTN
#undef PTM
#undef CSB
#undef CSR
#undef LXT
#undef UXT
#undef SBT
#undef CST
#undef CSF
#undef CSBT
#undef CSRT

/* Table for converting to lower-case */

const unsigned char u__ct_tol[256] = {
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

/* Table for converting to upper-case */

const unsigned char u__ct_tou[256] = {
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

// MIME TYPE

typedef struct mimeentry {
   const char* restrict type;
   const char* restrict name;
   uint32_t name_len;
} mimeentry;

#define MIME_ENTRY(name,type) { type, name+1, U_CONSTANT_SIZE(name)-1 }

/* Complete list of MIME types

.3dm  x-world/x-3dmf
.3dmf x-world/x-3dmf

.a    application/octet-stream
.aab  application/x-authorware-bin
.aam  application/x-authorware-map
.aas  application/x-authorware-seg
.abc  text/vnd.abc
.acgi text/html
.afl  video/animaflex
.ai   application/postscript
.aif  audio/aiff
.aif  audio/x-aiff
.aifc audio/aiff
.aifc audio/x-aiff
.aiff audio/aiff
.aiff audio/x-aiff
.aim  application/x-aim
.aip  text/x-audiosoft-intra
.ani  application/x-navi-animation
.aos  application/x-nokia-9000-communicator-add-on-software
.aps  application/mime
.arc  application/octet-stream
.arj  application/arj
.arj  application/octet-stream
.art  image/x-jg
.asf  video/x-ms-asf
.asm  text/x-asm
.asp  text/asp
.asx  application/x-mplayer2
.asx  video/x-ms-asf
.asx  video/x-ms-asf-plugin
.au   audio/basic
.au   audio/x-au
.avi  application/x-troff-msvideo
.avi  video/avi
.avi  video/msvideo
.avi  video/x-msvideo
.avs  video/avs-video

.bcpio application/x-bcpio
.bin  application/mac-binary
.bin  application/macbinary
.bin  application/octet-stream
.bin  application/x-binary
.bin  application/x-macbinary
.bm   image/bmp
.bmp  image/bmp
.bmp  image/x-windows-bmp
.boo  application/book
.book application/book
.boz  application/x-bzip2
.bsh  application/x-bsh
.bz   application/x-bzip
.bz2  application/x-bzip2

.c    text/plain
.c    text/x-c
.c++  text/plain
.cat  application/vnd.ms-pki.seccat
.cc   text/plain
.cc   text/x-c
.ccad application/clariscad
.cco  application/x-cocoa
.cdf  application/cdf
.cdf  application/x-cdf
.cdf  application/x-netcdf
.cer  application/pkix-cert
.cer  application/x-x509-ca-cert
.cha  application/x-chat
.chat application/x-chat
.class application/java
.class application/java-byte-code
.class application/x-java-class
.com  application/octet-stream
.com  text/plain
.conf text/plain
.cpio application/x-cpio
.cpp  text/x-c
.cpt  application/mac-compactpro
.cpt  application/x-compactpro
.cpt  application/x-cpt
.crl  application/pkcs-crl
.crl  application/pkix-crl
.crt  application/pkix-cert
.crt  application/x-x509-ca-cert
.crt  application/x-x509-user-cert
.csh  application/x-csh
.csh  text/x-script.csh
.css  application/x-pointplus
.css  text/css
.cxx  text/plain

.dcr  application/x-director
.deepv application/x-deepv
.def  text/plain
.der  application/x-x509-ca-cert
.dif  video/x-dv
.dir  application/x-director
.dl   video/dl
.dl   video/x-dl
.doc  application/msword
.dot  application/msword
.dp   application/commonground
.drw  application/drafting
.dump application/octet-stream
.dv   video/x-dv
.dvi  application/x-dvi
.dwf  drawing/x-dwf (old)
.dwf  model/vnd.dwf
.dwg  application/acad
.dwg  image/vnd.dwg
.dwg  image/x-dwg
.dxf  application/dxf
.dxf  image/vnd.dwg
.dxf  image/x-dwg
.dxr  application/x-director

.el   text/x-script.elisp
.elc  application/x-bytecode.elisp (compiled elisp)
.elc  application/x-elc
.env  application/x-envoy
.eps  application/postscript
.es   application/x-esrehber
.etx  text/x-setext
.evy  application/envoy
.evy  application/x-envoy
.exe  application/octet-stream

.f    text/plain
.f    text/x-fortran
.f77  text/x-fortran
.f90  text/plain
.f90  text/x-fortran
.fdf  application/vnd.fdf
.fif  application/fractals
.fif  image/fif
.fli  video/fli
.fli  video/x-fli
.flo  image/florian
.flx  text/vnd.fmi.flexstor
.fmf  video/x-atomic3d-feature
.for  text/plain
.for  text/x-fortran
.fpx  image/vnd.fpx
.fpx  image/vnd.net-fpx
.frl  application/freeloader
.funk audio/make

.g    text/plain
.g3   image/g3fax
.gif  image/gif
.gl   video/gl
.gl   video/x-gl
.gsd  audio/x-gsm
.gsm  audio/x-gsm
.gsp  application/x-gsp
.gss  application/x-gss
.gtar application/x-gtar
.gz   application/x-compressed
.gz   application/x-gzip
.gzip application/x-gzip
.gzip multipart/x-gzip

.h    text/plain
.h    text/x-h
.hdf  application/x-hdf
.help application/x-helpfile
.hgl  application/vnd.hp-hpgl
.hh   text/plain
.hh   text/x-h
.hlb  text/x-script
.hlp  application/hlp
.hlp  application/x-helpfile
.hlp  application/x-winhelp
.hpg  application/vnd.hp-hpgl
.hpgl application/vnd.hp-hpgl
.hqx  application/binhex
.hqx  application/binhex4
.hqx  application/mac-binhex
.hqx  application/mac-binhex40
.hqx  application/x-binhex40
.hqx  application/x-mac-binhex40
.hta  application/hta
.htc  text/x-component
.htm  text/html
.html text/html
.htmls text/html
.htt  text/webviewhtml
.htx  text/html

.ice  x-conference/x-cooltalk
.ico  image/x-icon
.idc  text/plain
.ief  image/ief
.iefs image/ief
.iges application/iges
.iges model/iges
.igs  application/iges
.igs  model/iges
.ima  application/x-ima
.imap application/x-httpd-imap
.inf  application/inf
.ins  application/x-internett-signup
.ip   application/x-ip2
.isu  video/x-isvideo
.it   audio/it
.iv   application/x-inventor
.ivr  i-world/i-vrml
.ivy  application/x-livescreen

.jam  audio/x-jam
.jav  text/plain
.jav  text/x-java-source
.java text/plain
.java text/x-java-source
.jcm  application/x-java-commerce
.jfif image/jpeg
.jfif image/pjpeg
.jfif-tbnl  image/jpeg
.jpe  image/jpeg
.jpe  image/pjpeg
.jpeg image/jpeg
.jpeg image/pjpeg
.jpg  image/jpeg
.jpg  image/pjpeg
.jps  image/x-jps
.js   application/x-javascript
.js   application/javascript
.js   application/ecmascript
.js   text/javascript
.js   text/ecmascript
.jut  image/jutvision

.kar  audio/midi
.kar  music/x-karaoke
.ksh  application/x-ksh
.ksh  text/x-script.ksh

.la   audio/nspaudio
.la   audio/x-nspaudio
.lam  audio/x-liveaudio
.latex application/x-latex
.lha  application/lha
.lha  application/octet-stream
.lha  application/x-lha
.lhx  application/octet-stream
.list text/plain
.lma  audio/nspaudio
.lma  audio/x-nspaudio
.log  text/plain
.lsp  application/x-lisp
.lsp  text/x-script.lisp
.lst  text/plain
.lsx  text/x-la-asf
.ltx  application/x-latex
.lzh  application/octet-stream
.lzh  application/x-lzh
.lzx  application/lzx
.lzx  application/octet-stream
.lzx  application/x-lzx

.m    text/plain
.m    text/x-m
.m1v  video/mpeg
.m2a  audio/mpeg
.m2v  video/mpeg
.m3u  audio/x-mpequrl
.man  application/x-troff-man
.map  application/x-navimap
.mar  text/plain
.mbd  application/mbedlet
.mc$  application/x-magic-cap-package-1.0
.mcd  application/mcad
.mcd  application/x-mathcad
.mcf  image/vasa
.mcf  text/mcf
.mcp  application/netmc
.me   application/x-troff-me
.mht  message/rfc822
.mhtml message/rfc822
.mid  application/x-midi
.mid  audio/midi
.mid  audio/x-mid
.mid  audio/x-midi
.mid  music/crescendo
.mid  x-music/x-midi
.midi application/x-midi
.midi audio/midi
.midi audio/x-mid
.midi audio/x-midi
.midi music/crescendo
.midi x-music/x-midi
.mif  application/x-frame
.mif  application/x-mif
.mime message/rfc822
.mime www/mime
.mjf  audio/x-vnd.audioexplosion.mjuicemediafile
.mjpg video/x-motion-jpeg
.mm   application/base64
.mm   application/x-meme
.mme  application/base64
.mod  audio/mod
.mod  audio/x-mod
.moov video/quicktime
.mov  video/quicktime
.movie video/x-sgi-movie
.mp2  audio/mpeg
.mp2  audio/x-mpeg
.mp2  video/mpeg
.mp2  video/x-mpeg
.mp2  video/x-mpeq2a
.mp3  audio/mpeg3
.mp3  audio/x-mpeg-3
.mp3  video/mpeg
.mp3  video/x-mpeg
.mpa  audio/mpeg
.mpa  video/mpeg
.mpc  application/x-project
.mpe  video/mpeg
.mpeg video/mpeg
.mpg  audio/mpeg
.mpg  video/mpeg
.mpga audio/mpeg
.mpp  application/vnd.ms-project
.mpt  application/x-project
.mpv  application/x-project
.mpx  application/x-project
.mrc  application/marc
.ms   application/x-troff-ms
.mv   video/x-sgi-movie
.my   audio/make
.mzz  application/x-vnd.audioexplosion.mzz

.nap  image/naplps
.naplps image/naplps
.nc   application/x-netcdf
.ncm  application/vnd.nokia.configuration-message
.nif  image/x-niff
.niff image/x-niff
.nix  application/x-mix-transfer
.nsc  application/x-conference
.nvd  application/x-navidoc

.o    application/octet-stream
.oda  application/oda
.omc  application/x-omc
.omcd application/x-omcdatamaker
.omcr application/x-omcregerator

.p    text/x-pascal
.p10  application/pkcs10
.p10  application/x-pkcs10
.p12  application/pkcs-12
.p12  application/x-pkcs12
.p7a  application/x-pkcs7-signature
.p7c  application/pkcs7-mime
.p7c  application/x-pkcs7-mime
.p7m  application/pkcs7-mime
.p7m  application/x-pkcs7-mime
.p7r  application/x-pkcs7-certreqresp
.p7s  application/pkcs7-signature
.part application/pro_eng
.pas  text/pascal
.pbm  image/x-portable-bitmap
.pcl  application/vnd.hp-pcl
.pcl  application/x-pcl
.pct  image/x-pict
.pcx  image/x-pcx
.pdb  chemical/x-pdb
.pdf  application/pdf
.pfunk audio/make
.pfunk audio/make.my.funk
.pgm  image/x-portable-graymap
.pgm  image/x-portable-greymap
.pic  image/pict
.pict image/pict
.pkg  application/x-newton-compatible-pkg
.pko  application/vnd.ms-pki.pko
.pl   text/plain
.pl   text/x-script.perl
.plx  application/x-pixclscript
.pm   image/x-xpixmap
.pm   text/x-script.perl-module
.pm4  application/x-pagemaker
.pm5  application/x-pagemaker
.png  image/png
.pnm  application/x-portable-anymap
.pnm  image/x-portable-anymap
.pot  application/mspowerpoint
.pot  application/vnd.ms-powerpoint
.pov  model/x-pov
.ppa  application/vnd.ms-powerpoint
.ppm  image/x-portable-pixmap
.pps  application/mspowerpoint
.pps  application/vnd.ms-powerpoint
.ppt  application/mspowerpoint
.ppt  application/powerpoint
.ppt  application/vnd.ms-powerpoint
.ppt  application/x-mspowerpoint
.ppz  application/mspowerpoint
.pre  application/x-freelance
.prt  application/pro_eng
.ps   application/postscript
.psd  application/octet-stream
.pvu  paleovu/x-pv
.pwz  application/vnd.ms-powerpoint
.py   text/x-script.phyton
.pyc  applicaiton/x-bytecode.python

.qcp  audio/vnd.qcelp
.qd3  x-world/x-3dmf
.qd3d x-world/x-3dmf
.qif  image/x-quicktime
.qt   video/quicktime
.qtc  video/x-qtc
.qti  image/x-quicktime
.qtif image/x-quicktime

.ra   audio/x-pn-realaudio
.ra   audio/x-pn-realaudio-plugin
.ra   audio/x-realaudio
.ram  audio/x-pn-realaudio
.ras  application/x-cmu-raster
.ras  image/cmu-raster
.ras  image/x-cmu-raster
.rast image/cmu-raster
.rexx text/x-script.rexx
.rf   image/vnd.rn-realflash
.rgb  image/x-rgb
.rm   application/vnd.rn-realmedia
.rm   audio/x-pn-realaudio
.rmi  audio/mid
.rmm  audio/x-pn-realaudio
.rmp  audio/x-pn-realaudio
.rmp  audio/x-pn-realaudio-plugin
.rng  application/ringing-tones
.rng  application/vnd.nokia.ringing-tone
.rnx  application/vnd.rn-realplayer
.roff application/x-troff
.rp   image/vnd.rn-realpix
.rpm  audio/x-pn-realaudio-plugin
.rt   text/richtext
.rt   text/vnd.rn-realtext
.rtf  application/rtf
.rtf  application/x-rtf
.rtf  text/richtext
.rtx  application/rtf
.rtx  text/richtext
.rv   video/vnd.rn-realvideo

.s    text/x-asm
.s3m  audio/s3m
.saveme application/octet-stream
.sbk  application/x-tbook
.scm  application/x-lotusscreencam
.scm  text/x-script.guile
.scm  text/x-script.scheme
.scm  video/x-scm
.sdml text/plain
.sdp  application/sdp
.sdp  application/x-sdp
.sdr  application/sounder
.sea  application/sea
.sea  application/x-sea
.set  application/set
.sgm  text/sgml
.sgm  text/x-sgml
.sgml text/sgml
.sgml text/x-sgml
.sh   application/x-bsh
.sh   application/x-sh
.sh   application/x-shar
.sh   text/x-script.sh
.shar application/x-bsh
.shar application/x-shar
.shtml text/html
.shtml text/x-server-parsed-html
.sid  audio/x-psid
.sit  application/x-sit
.sit  application/x-stuffit
.skd  application/x-koan
.skm  application/x-koan
.skp  application/x-koan
.skt  application/x-koan
.sl   application/x-seelogo
.smi  application/smil
.smil application/smil
.snd  audio/basic
.snd  audio/x-adpcm
.sol  application/solids
.spc  application/x-pkcs7-certificates
.spc  text/x-speech
.spl  application/futuresplash
.spr  application/x-sprite
.sprite application/x-sprite
.src  application/x-wais-source
.ssi  text/x-server-parsed-html
.ssm  application/streamingmedia
.sst  application/vnd.ms-pki.certstore
.step application/step
.stl  application/sla
.stl  application/vnd.ms-pki.stl
.stl  application/x-navistyle
.stp  application/step
.sv4cpio application/x-sv4cpio
.sv4crc  application/x-sv4crc
.svf  image/vnd.dwg
.svf  image/x-dwg
.svr  application/x-world
.svr  x-world/x-svr
.swf  application/x-shockwave-flash

.t    application/x-troff
.talk text/x-speech
.tar  application/x-tar
.tbk  application/toolbook
.tbk  application/x-tbook
.tcl  application/x-tcl
.tcl  text/x-script.tcl
.tcsh text/x-script.tcsh
.tex  application/x-tex
.texi application/x-texinfo
.texinfo application/x-texinfo
.text application/plain
.text text/plain
.tgz  application/gnutar
.tgz  application/x-compressed
.tif  image/tiff
.tif  image/x-tiff
.tiff image/tiff
.tiff image/x-tiff
.tr   application/x-troff
.tsi  audio/tsp-audio
.tsp  application/dsptype
.tsp  audio/tsplayer
.tsv  text/tab-separated-values
.turbot image/florian
.txt  text/plain

.uil  text/x-uil
.uni  text/uri-list
.unis text/uri-list
.unv  application/i-deas
.uri  text/uri-list
.uris text/uri-list
.ustar application/x-ustar
.ustar multipart/x-ustar
.uu   application/octet-stream
.uu   text/x-uuencode
.uue  text/x-uuencode

.vcd  application/x-cdlink
.vcs  text/x-vcalendar
.vda  application/vda
.vdo  video/vdo
.vew  application/groupwise
.viv  video/vivo
.viv  video/vnd.vivo
.vivo video/vivo
.vivo video/vnd.vivo
.vmd  application/vocaltec-media-desc
.vmf  application/vocaltec-media-file
.voc  audio/voc
.voc  audio/x-voc
.vos  video/vosaic
.vox  audio/voxware
.vqe  audio/x-twinvq-plugin
.vqf  audio/x-twinvq
.vql  audio/x-twinvq-plugin
.vrml application/x-vrml
.vrml model/vrml
.vrml x-world/x-vrml
.vrt  x-world/x-vrt
.vsd  application/x-visio
.vst  application/x-visio
.vsw  application/x-visio

.w60  application/wordperfect6.0
.w61  application/wordperfect6.1
.w6w  application/msword
.wav  audio/wav
.wav  audio/x-wav
.wb1  application/x-qpro
.wbmp image/vnd.wap.wbmp
.web  application/vnd.xara
.wiz  application/msword
.wk1  application/x-123
.wmf  windows/metafile
.wml  text/vnd.wap.wml
.wmlc application/vnd.wap.wmlc
.wmls text/vnd.wap.wmlscript
.wmlsc application/vnd.wap.wmlscriptc
.word application/msword
.wp   application/wordperfect
.wp5  application/wordperfect
.wp5  application/wordperfect6.0
.wp6  application/wordperfect
.wpd  application/wordperfect
.wpd  application/x-wpwin
.wq1  application/x-lotus
.wri  application/mswrite
.wri  application/x-wri
.wrl  application/x-world
.wrl  model/vrml
.wrl  x-world/x-vrml
.wrz  model/vrml
.wrz  x-world/x-vrml
.wsc  text/scriplet
.wsrc application/x-wais-source
.wtk  application/x-wintalk

.xbm  image/x-xbitmap
.xbm  image/x-xbm
.xbm  image/xbm
.xdr  video/x-amt-demorun
.xgz  xgl/drawing
.xif  image/vnd.xiff
.xl   application/excel
.xla  application/excel
.xla  application/x-excel
.xla  application/x-msexcel
.xlb  application/excel
.xlb  application/vnd.ms-excel
.xlb  application/x-excel
.xlc  application/excel
.xlc  application/vnd.ms-excel
.xlc  application/x-excel
.xld  application/excel
.xld  application/x-excel
.xlk  application/excel
.xlk  application/x-excel
.xll  application/excel
.xll  application/vnd.ms-excel
.xll  application/x-excel
.xlm  application/excel
.xlm  application/vnd.ms-excel
.xlm  application/x-excel
.xls  application/excel
.xls  application/vnd.ms-excel
.xls  application/x-excel
.xls  application/x-msexcel
.xlt  application/excel
.xlt  application/x-excel
.xlv  application/excel
.xlv  application/x-excel
.xlw  application/excel
.xlw  application/vnd.ms-excel
.xlw  application/x-excel
.xlw  application/x-msexcel
.xm   audio/xm
.xml  application/xml
.xml  text/xml
.xmz  xgl/movie
.xpix application/x-vnd.ls-xpix
.xpm  image/x-xpixmap
.xpm  image/xpm
.x-png image/png
.xsr  video/x-amt-showrun
.xwd  image/x-xwd
.xwd  image/x-xwindowdump
.xyz  chemical/x-pdb

.z    application/x-compress
.z    application/x-compressed
.zip  application/x-compressed
.zip  application/x-zip-compressed
.zip  application/zip
.zip  multipart/x-zip
.zoo  application/octet-stream
.zsh  text/x-script.zsh
*/

static struct mimeentry mimetab_a[] = {
   MIME_ENTRY( "ai",       "application/postscript" ),
   MIME_ENTRY( "arj",      "application/x-arj-compressed" ),
   MIME_ENTRY( "asx",      "video/x-ms-asf" ),
   MIME_ENTRY( "atom",     "application/atom+xml" ),
   MIME_ENTRY( "avi",      "video/x-msvideo" ),
   MIME_ENTRY( "appcache", "text/cache-manifest" ),

   /*
   MIME_ENTRY( "aif",  "audio/x-aiff" ), (aifc, aiff)
   MIME_ENTRY( "api",  "application/postscript" ),
   MIME_ENTRY( "asc",  "text/plain" ),
   MIME_ENTRY( "au",   "audio/basic" ),
   */

   { 0, 0, 0 }
};

static struct mimeentry mimetab_b[] = {
   MIME_ENTRY( "bat",  "application/x-msdos-program" ),
   MIME_ENTRY( "bmp",  "image/x-ms-bmp" ),
   MIME_ENTRY( "bild", "image/jpeg" ),
   MIME_ENTRY( "boz",  "application/x-bzip2" ),

   /*
   MIME_ENTRY( "bin",   "application/x-bcpio" ),
   MIME_ENTRY( "bcpio", "application/octet-stream" ),
   */

   { 0, 0, 0 }
};

static struct mimeentry mimetab_c[] = {
   /* -------------------------------------------------------------------------------
   * The certificate being downloaded represents a Certificate Authority.
   * When it is downloaded the user will be shown a sequence of dialogs that
   * will guide them through the process of accepting the Certificate Authority
   * and deciding if they wish to trust sites certified by the CA. If a certificate
   * chain is being imported then the first certificate in the chain must be the CA
   * certificate, and any subsequent certificates will be added as untrusted CA
   * certificates to the local database.
   * ------------------------------------------------------------------------------- */
   MIME_ENTRY( "css", "text/css" ), /* U_css */
   MIME_ENTRY( "crt", "application/x-x509-ca-cert" ),
   MIME_ENTRY( "cer", "application/x-x509-ca-cert" ),
   MIME_ENTRY( "crx", "application/x-chrome-extension crx" ),

   /*
   MIME_ENTRY( "c++",     "text/x-c++src" ),
   MIME_ENTRY( "c4d",     "application/vnd.clonk.c4group" ),
   MIME_ENTRY( "cac",     "chemical/x-cache" ),
   MIME_ENTRY( "cascii",  "chemical/x-cactvs-binary" ),
   MIME_ENTRY( "cct",     "application/x-director" ),
   MIME_ENTRY( "cdf",     "application/x-netcdf" ),
   MIME_ENTRY( "cef",     "chemical/x-cxf" ),
   MIME_ENTRY( "cls",     "text/x-tex" ),
   MIME_ENTRY( "cpio",    "application/x-cpio" ),
   MIME_ENTRY( "cpt",     "application/mac-compactpro" ),
   MIME_ENTRY( "csm",     "chemical/x-csml" ),
   MIME_ENTRY( "csh",     "application/x-csh" ),
   MIME_ENTRY( "cdf",     "application/x-netcdf" ),
   MIME_ENTRY( "c",       "text/x-c" ),
   */

   { 0, 0, 0 }
};

static struct mimeentry mimetab_d[] = {
   /* -------------------------------------------------------------------------------
   * The certificate being downloaded represents a Certificate Authority.
   * When it is downloaded the user will be shown a sequence of dialogs that
   * will guide them through the process of accepting the Certificate Authority
   * and deciding if they wish to trust sites certified by the CA. If a certificate
   * chain is being imported then the first certificate in the chain must be the CA
   * certificate, and any subsequent certificates will be added as untrusted CA
   * certificates to the local database.
   * ------------------------------------------------------------------------------- */
   MIME_ENTRY( "der", "application/x-x509-ca-cert" ),
   MIME_ENTRY( "doc", "application/msword" ),
   MIME_ENTRY( "dtd", "application/xml" ),

   /*
   MIME_ENTRY( "dcr", "application/x-director" ),
   MIME_ENTRY( "deb", "application/x-debian-package" ),
   MIME_ENTRY( "dir", "application/x-director" ),
   MIME_ENTRY( "dll", "application/octet-stream" ),
   MIME_ENTRY( "dms", "application/octet-stream" ),
   MIME_ENTRY( "dvi", "application/x-dvi" ),
   MIME_ENTRY( "dxr", "application/x-director" ),
   */

   { 0, 0, 0 }
};

static struct mimeentry mimetab_e[] = {
   MIME_ENTRY( "eps", "application/postscript" ),
   MIME_ENTRY( "eot", "application/vnd.ms-fontobject" ),

   /*
   MIME_ENTRY( "etx", "text/x-setext" ),
   MIME_ENTRY( "ez",  "application/andrew-inset" ),
   MIME_ENTRY( "exe", "application/octet-stream" ),
   */

   { 0, 0, 0 }
};

static struct mimeentry mimetab_f[] = {
   MIME_ENTRY( "flv", "video/x-flv" ),
   { 0, 0, 0 }
};

static struct mimeentry mimetab_g[] = {
   MIME_ENTRY( "gif",  "image/gif" ),  /* U_gif */
   MIME_ENTRY( "gtar", "application/x-gtar" ),

   /*
   MIME_ENTRY( "gz",   "application/x-gzip" ),
   */

   { 0, 0, 0 }
};

static struct mimeentry mimetab_h[] = {
   MIME_ENTRY( "htm", U_CTYPE_HTML ),  /* (html) U_html */
   MIME_ENTRY( "htc", "text/x-component" ),

   /*
   MIME_ENTRY( "hdf", "application/x-hdf" ),
   MIME_ENTRY( "hqx", "application/mac-binhex40" ),
   */

   { 0, 0, 0 }
};

static struct mimeentry mimetab_i[] = {
   MIME_ENTRY( "ico", U_CTYPE_ICO ),

   /*
   MIME_ENTRY( "ice", "x-conference/x-cooltalk" ),
   MIME_ENTRY( "ief", "image/ief" ),
   MIME_ENTRY( "ig",  "model/iges" ), (igs, iges) 
   */

   { 0, 0, 0 }
};

static struct mimeentry mimetab_j[] = {
   MIME_ENTRY( "js",   "text/javascript" ), /* U_js */
   MIME_ENTRY( "json", "application/json" ),
   MIME_ENTRY( "jp",   "image/jpeg" ),      /* (jpg, jpe, jpeg) U_jpg */
   { 0, 0, 0 }
};

/*
static struct mimeentry mimetab_k[] = {
   MIME_ENTRY( "kar", "audio/midi" ),
   { 0, 0, 0 }
};

static struct mimeentry mimetab_l[] = {
   MIME_ENTRY( "latex", "application/x-latex" ),
   MIME_ENTRY( "lh",    "application/octet-stream" ), (lha, lhz)
   { 0, 0, 0 }
};
*/

static struct mimeentry mimetab_m[] = {
   MIME_ENTRY( "mng",      "video/x-mng" ),
   MIME_ENTRY( "mp4",      "video/mp4" ),
   MIME_ENTRY( "m4a",      "audio/mp4" ),
   MIME_ENTRY( "mp",       "video/mpeg" ), /* (mp2, mp3, mpg, mpe, mpeg, mpga) */
   MIME_ENTRY( "md5",      "text/plain" ),
   MIME_ENTRY( "mov",      "video/quicktime" ),
   MIME_ENTRY( "mf",       "text/cache-manifest" ),
   MIME_ENTRY( "manifest", "text/cache-manifest" ),

   /*
   MIME_ENTRY( "m3u",  "audio/x-mpegurl" ),
   MIME_ENTRY( "man",  "application/x-troff-man" ),
   MIME_ENTRY( "me",   "application/x-troff-me" ),
   MIME_ENTRY( "mesh", "model/mesh" ),
   MIME_ENTRY( "msh",  "model/mesh" ),
   MIME_ENTRY( "mif",  "application/vnd.mif" ),
   MIME_ENTRY( "mi",   "audio/midi" ), (mid, midi)
   MIME_ENTRY( "movie","video/x-sgi-movie" ),
   MIME_ENTRY( "ms",   "application/x-troff-ms" ),
   */

   { 0, 0, 0 }
};

/*
static struct mimeentry mimetab_n[] = {
   MIME_ENTRY( "nc", "application/x-netcdf" ),
   { 0, 0, 0 }
};
*/

static struct mimeentry mimetab_o[] = {
   MIME_ENTRY( "ogv", "video/ogg" ),
   MIME_ENTRY( "og",  "audio/ogg" ), /* (oga, ogg) */
   MIME_ENTRY( "otf", "font/opentype" ),

   /*
   MIME_ENTRY( "oda",  "application/oda" ),
   */

   { 0, 0, 0 }
};

static struct mimeentry mimetab_p[] = {
   MIME_ENTRY( "png", "image/png" ),   /* U_png */
   MIME_ENTRY( "pdf", "application/pdf" ),
   MIME_ENTRY( "pem", "application/x-x509-ca-cert" ),
   MIME_ENTRY( "php", "application/x-httpd-php" ),
   MIME_ENTRY( "pbm", "image/x-portable-bitmap" ),
   MIME_ENTRY( "pgm", "image/x-portable-graymap" ),
   MIME_ENTRY( "pnm", "image/x-portable-anymap" ),
   MIME_ENTRY( "ppm", "image/x-portable-pixmap" ),
   MIME_ENTRY( "ps",  "application/postscript" ),
   MIME_ENTRY( "p7b", "application/x-pkcs7-certificates" ),
   MIME_ENTRY( "p7c", "application/x-pkcs7-mime" ),
   MIME_ENTRY( "p12", "application/x-pkcs12" ),
   MIME_ENTRY( "ppt", "application/vnd.ms-powerpoint" ),

   /*
   MIME_ENTRY( "pac", "application/x-ns-proxy-autoconfig" ),
   MIME_ENTRY( "pdb", "chemical/x-pdb" ),
   MIME_ENTRY( "pgn", "application/x-chess-pgn" ),
   */

   { 0, 0, 0 }
};

static struct mimeentry mimetab_q[] = {
   MIME_ENTRY( "qt", "video/quicktime" ),
   { 0, 0, 0 }
};

static struct mimeentry mimetab_r[] = {
   MIME_ENTRY( "rar",  "application/x-rar-compressed" ),
   MIME_ENTRY( "rtf",  "text/rtf" ),
   MIME_ENTRY( "rtx",  "text/richtext" ),
   MIME_ENTRY( "rdf",  "application/rdf+xml" ),
   MIME_ENTRY( "roff", "application/x-troff" ),
   MIME_ENTRY( "rss",  "application/rss+xml" ),
   MIME_ENTRY( "xrdf", "application/rdf+xml" ),

   /*
   MIME_ENTRY( "ra",  "audio/x-realaudio" ),
   MIME_ENTRY( "ras", "image/x-cmu-raster" ),
   MIME_ENTRY( "rgb", "image/x-rgb" ),
   MIME_ENTRY( "rpm", "audio/x-pn-realaudio-plugin" ),
   MIME_ENTRY( "ram", "audio/x-pn-realaudio" ),
   MIME_ENTRY( "rm",  "audio/x-pn-realaudio" ),
   */

   { 0, 0, 0 }
};

static struct mimeentry mimetab_s[] = {
   MIME_ENTRY( "shtm",       U_CTYPE_HTML ), /* (shtml) U_ssi */
   MIME_ENTRY( "svg",        "image/svg+xml" ), /* (svg, svgz) */
   MIME_ENTRY( "swf",        "application/x-shockwave-flash" ),
   MIME_ENTRY( "sgm",        "text/sgml" ), /* (sgml) */
   MIME_ENTRY( "safariextz", "application/octet-stream" ),

   /*
   MIME_ENTRY( "sh",      "application/x-sh" ),
   MIME_ENTRY( "snd",     "audio/basic" ),
   MIME_ENTRY( "shar",    "application/x-shar" ),
   MIME_ENTRY( "sit",     "application/x-stuffit" ),
   MIME_ENTRY( "silo",    "model/mesh" ),
   MIME_ENTRY( "sig",     "application/pgp-signature" ),
   MIME_ENTRY( "spl",     "application/x-futuresplash" ),
   MIME_ENTRY( "src",     "application/x-wais-source" ),
   MIME_ENTRY( "smi",     "application/smil" ), (smil)
   MIME_ENTRY( "sk",      "application/x-koan" ), (skd, skm, skp, skt)
   MIME_ENTRY( "sv4cpio", "application/x-sv4cpio" ),
   MIME_ENTRY( "sv4crc",  "application/x-sv4crc" ),
   */

   { 0, 0, 0 }
};

static struct mimeentry mimetab_t[] = {
   MIME_ENTRY( "tar",  "application/x-tar" ),
   MIME_ENTRY( "tif",  "image/tiff" ), /* (tiff) */
   MIME_ENTRY( "text", "text/plain" ),
   MIME_ENTRY( "txt",  "text/plain" ),
   MIME_ENTRY( "tgz",  "application/x-tar-gz" ),
   MIME_ENTRY( "ttf",  "font/truetype" ),
   MIME_ENTRY( "ttl",  "text/turtle" ),

   /*
   MIME_ENTRY( "texi",     "application/x-texinfo" ), (texinfo)
   MIME_ENTRY( "tex",      "application/x-tex" ),
   MIME_ENTRY( "tr",       "application/x-troff" ),
   MIME_ENTRY( "tcl",      "application/x-tcl" ),
   MIME_ENTRY( "tsv",      "text/tab-separated-values" ),
   MIME_ENTRY( "torrent",  "application/x-bittorrent" ),
   */

   { 0, 0, 0 }
};

static struct mimeentry mimetab_u[] = {
   MIME_ENTRY( "usp", "text/plain" ),

   /*
   MIME_ENTRY( "untar",  "application/x-ustar" ),
   */

   { 0, 0, 0 }
};

/*
static struct mimeentry mimetab_v[] = {
   MIME_ENTRY( "vbxml", "application/vnd.wap.wbxml" ),
   MIME_ENTRY( "vcd",   "application/x-cdlink" ),
   MIME_ENTRY( "vmrl",  "model/vrml" ),
   { 0, 0, 0 }
};
*/

static struct mimeentry mimetab_w[] = {
   MIME_ENTRY( "wav",  "audio/x-wav" ),
   MIME_ENTRY( "wmv",  "video/x-ms-wmv" ),
   MIME_ENTRY( "webm", "video/webm" ),
   MIME_ENTRY( "woff", "application/x-font-woff" ),
   MIME_ENTRY( "webp", "image/webp" ),

   /*
   MIME_ENTRY( "wbmp",  "image/vnd.wap.wbmp" ),
   MIME_ENTRY( "wml",   "text/vnd.wap.wml" ),
   MIME_ENTRY( "wmls",  "text/vnd.wap.wmlscript" ),
   MIME_ENTRY( "wmlc",  "application/vnd.wap.wmlc" ),
   MIME_ENTRY( "wmlsc", "application/vnd.wap.wmlscriptc" ),
   MIME_ENTRY( "wrl",   "model/vrml" ),
   */

   { 0, 0, 0 }
};

static struct mimeentry mimetab_x[] = {
   MIME_ENTRY( "xls", "application/vnd.ms-excel" ),
   MIME_ENTRY( "xml", "text/xml" ),
   MIME_ENTRY( "xsl", "text/xml" ),
   MIME_ENTRY( "xpm", "image/x-xpixmap" ),
   MIME_ENTRY( "xbm", "image/x-xbitmap" ),
   MIME_ENTRY( "xpi", "application/x-xpinstall" ),

   /*
   MIME_ENTRY( "xyz", "chemical/x-pdb" ),
   MIME_ENTRY( "xwd", "image/x-xwindowdump" ),
   */

   { 0, 0, 0 }
};

static struct mimeentry mimetab_z[] = {
   MIME_ENTRY( "zip", "application/zip" ),
   { 0, 0, 0 }
};

static struct mimeentry mimetab_null[] = {
   { 0, 0, 0 }
};

const char* u_get_mimetype(const char* restrict suffix)
{
   char c;
   uint32_t i;
   ptrdiff_t diff;
   struct mimeentry* ptr;
   struct mimeentry* mimetab;

   U_INTERNAL_TRACE("u_get_mimetype(%s)", suffix)

   U_INTERNAL_ASSERT_POINTER(suffix)

   switch ((c = *suffix++))
      {
      case 'a': mimetab = mimetab_a; break;
      case 'b': mimetab = mimetab_b; break;
      case 'c': mimetab = mimetab_c; break;
      case 'd': mimetab = mimetab_d; break;
      case 'e': mimetab = mimetab_e; break;
      case 'f': mimetab = mimetab_f; break;
      case 'g': mimetab = mimetab_g; break;
      case 'h': mimetab = mimetab_h; break;
      case 'i': mimetab = mimetab_i; break;
      case 'j': mimetab = mimetab_j; break;
   /* case 'k': mimetab = mimetab_null; break; */
   /* case 'l': mimetab = mimetab_null; break; */
      case 'm': mimetab = mimetab_m; break;
   /* case 'n': mimetab = mimetab_null; break; */
      case 'o': mimetab = mimetab_o; break;
      case 'p': mimetab = mimetab_p; break;
      case 'q': mimetab = mimetab_q; break;
      case 'r': mimetab = mimetab_r; break;
      case 's': mimetab = mimetab_s; break;
      case 't': mimetab = mimetab_t; break;
      case 'u': mimetab = mimetab_u; break;
   /* case 'v': mimetab = mimetab_null; break; */
      case 'w': mimetab = mimetab_w; break;
      case 'x': mimetab = mimetab_x; break;
      case 'z': mimetab = mimetab_z; break;
      default:  mimetab = mimetab_null;
      }

   U_INTERNAL_PRINT("c = %d", c)

   ptr = mimetab;

   loop:
   while (ptr->name)
      {
      U_INTERNAL_PRINT("mimetab = %p (%s,%u,%s)", ptr, ptr->name, ptr->name_len, ptr->type)

      for (i = 0; i < ptr->name_len; ++i)
         {
         if (suffix[i] != ptr->name[i])
            {
            ++ptr;

            goto loop;
            }
         }

      diff = ptr - mimetab;

      U_INTERNAL_PRINT("diff = %u sizeof(mimeentry) = %u", diff, sizeof(mimeentry))

           if (diff ==  0)                       u_mime_index =  c;  /* NB: 1 entry: c(U_css), j(U_js), h(U_html), g(U_gif), f(U_flv), p(U_png), s(U_ssi), u(U_usp) */
      else if (diff == (ptrdiff_t)2 && c == 'j') u_mime_index = 'J'; /* NB: 3 entry: U_jpg */

      U_INTERNAL_PRINT("u_mime_index = %d", u_mime_index)

      return ptr->type;
      }

   return 0;
}
