/** ============================================================================
//
// = LIBRARY
//    ulibase - c library
//
// = FILENAME
//    utility.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================ */

#ifndef ULIBASE_UTILITY_H
#define ULIBASE_UTILITY_H 1

#include <ulib/base/base.h>

#include <stdlib.h>

#ifdef HAVE_SCHED_H
#  include <sched.h>
#endif

#ifndef CPU_SETSIZE
typedef uint64 cpu_set_t;
#  define CPU_SETSIZE                   (sizeof(cpu_set_t) * 8)
#  define CPU_ISSET(index, cpu_set_ptr) (*(cpu_set_ptr)  &  (1ULL << (index)))
#  define CPU_SET(index, cpu_set_ptr)   (*(cpu_set_ptr) |=  (1ULL << (index)))
#  define CPU_ZERO(cpu_set_ptr)         (*(cpu_set_ptr)  = 0)
#  define CPU_CLR(index, cpu_set_ptr)   (*(cpu_set_ptr) &= ~(1ULL << (index)))
#  define CPUSET_BITS(set) (set)
#else
#  define CPUSET_BITS(set) ((set)->__bits)
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Services */

union uuaddress {
   uint8_t        inaddr[4];
   uint32_t       ip;
   struct in_addr addr;
};

extern U_EXPORT const char* u_short_units[]; /* { "B", "KB", "MB", "GB", "TB", 0 } */

extern U_EXPORT char*       u_inet_nstoa(uint8_t* ip);
extern U_EXPORT char*       u_inet_nltoa(uint32_t ip);
extern U_EXPORT int         u_getScreenWidth(void) __pure; /* Determine the width of the terminal we're running on */
extern U_EXPORT int         u_get_num_random(int range);
extern U_EXPORT const char* u_get_mimetype(const char* restrict suffix);
extern U_EXPORT bool        u_isNumber(const char* restrict s, uint32_t n) __pure;
extern U_EXPORT void        u_printSize(char* restrict buffer, uint64_t bytes); /* print size using u_calcRate() */
extern U_EXPORT uint32_t    u_findEndHeader(const char* restrict s, uint32_t n); /* find sequence of U_LF2 or U_CRLF2 */
extern U_EXPORT char*       u_getPathRelativ(const char* restrict path, uint32_t* restrict path_len);
extern U_EXPORT double      u_calcRate(uint64_t bytes, uint32_t msecs, int* restrict units); /* Calculate the transfert rate */
extern U_EXPORT bool        u_rmatch(const char* restrict haystack, uint32_t haystack_len, const char* restrict needle, uint32_t needle_len) __pure;

#if defined(HAVE_MEMMEM) && !defined(__USE_GNU)
extern U_EXPORT void* memmem(const void* restrict haystack, size_t haystacklen, const void* restrict needle, size_t needlelen);
#endif

#ifdef DEBUG
extern U_EXPORT size_t u_strlen(const char* restrict s);
extern U_EXPORT char*  u_strcpy( char* restrict dest, const char* restrict src);
extern U_EXPORT void*  u_memcpy( void* restrict dest, const void* restrict src, size_t n);
extern U_EXPORT char*  u_strncpy(char* restrict dest, const char* restrict src, size_t n);
#else
#  define u_strlen(s)           strlen((s))
#  define u_strcpy(dest,src)    strcpy( (dest),(src))
#  define u_memcpy(dest,src,n)  memcpy( (dest),(src),(n))
#  define u_strncpy(dest,src,n) strncpy((dest),(src),(n))
#endif

static inline int u_equal(const void* restrict s1, const void* restrict s2, uint32_t n, bool ignore_case) /* Equal with ignore case */
{
   U_INTERNAL_TRACE("u_equal(%p,%p,%u)", s1, s2, n)

   U_INTERNAL_ASSERT_MAJOR(n,0)
   U_INTERNAL_ASSERT_POINTER(s1)
   U_INTERNAL_ASSERT_POINTER(s2)

   return (ignore_case ? strncasecmp((const char*)s1, (const char*)s2, n)
                       :      memcmp(             s1,              s2, n));
}

extern U_EXPORT void* u_find(const char* restrict s, uint32_t n, const char* restrict a, uint32_t n1) __pure;

/* check if string a start with string b */

extern U_EXPORT bool u_startsWith(const char* restrict a, uint32_t n1, const char* restrict b, uint32_t n2) __pure;

/* check if string a terminate with string b */

extern U_EXPORT bool u_endsWith(const char* restrict a, uint32_t n1, const char* restrict b, uint32_t n2) __pure;

/* find char not quoted */

extern U_EXPORT const char* u_find_char(const char* restrict s, const char* restrict end, char c) __pure;

/* skip string delimiter or white space and line comment */

extern U_EXPORT const char* u_skip(const char* restrict s, const char* restrict end, const char* restrict delim, char line_comment) __pure;

/* delimit token */

extern U_EXPORT const char* u_delimit_token(const char* restrict s, const char** restrict p, const char* restrict end, const char* restrict delim, char skip_line_comment);

/* Search a string for any of a set of characters. Locates the first occurrence in the string s of any of the characters in the string accept */

extern U_EXPORT const char* u_strpbrk(const char* restrict s, uint32_t slen, const char* restrict accept) __pure;

/* Search a string for a terminator of a group of delimitator {} [] () <%%>...*/

extern U_EXPORT const char* u_strpend(const char* restrict s, uint32_t slen,
                                      const char* restrict group_delimitor, uint32_t group_delimitor_len,
                                      char skip_line_comment) __pure;

/* WILDCARD PATTERN - The rules are as follows (POSIX.2, 3.13).

Wildcard Matching
 A string is a wildcard pattern if it contains one of the characters '?', '*' or '['. Globbing is the operation that
 expands a wildcard pattern into the list of pathnames matching the pattern. Matching is defined by:

 A '?' (not between brackets) matches any single character.
 A '*' (not between brackets) matches any string, including the empty string.

Character classes
 An expression '[...]' where the first character after the leading '[' is not an '!' matches a single character, namely
 any of the characters enclosed by the brackets. The string enclosed by the brackets cannot be empty; therefore ']' can
 be allowed between the brackets, provided that it is the first character. (Thus, '[][!]' matches the three characters
 '[', ']' and '!'.)

Ranges
 There is one special convention: two characters separated by '-' denote a range. (Thus, '[A-Fa-f0-9]' is equivalent to
 '[ABCDEFabcdef0123456789]'.) One may include '-' in its literal meaning by making it the first or last character
 between the brackets. (Thus, '[]-]' matches just the two characters ']' and '-', and '[--0]' matches the three characters
 '-', '.', '0', since '/' cannot be matched.)

Complementation
 An expression '[!...]' matches a single character, namely any character that is not matched by the expression obtained
 by removing the first '!' from it. (Thus, '[!]a-]' matches any single character except ']', 'a' and '-'.)

 One can remove the special meaning of '?', '*' and '[' by preceding them by a backslash, or, in case this is part of a
 shell command line, enclosing them in quotes. Between brackets these characters stand for themselves. Thus, '[[?*\]'
 matches the four characters '[', '?', '*' and '\'.

Pathnames
 Globbing is applied on each of the components of a pathname separately. A '/' in a pathname cannot be matched by a '?'
 or '*'  wildcard, or by a range like '[.-0]'. A range cannot contain an explicit '/' character; this would lead to a
 syntax error.

 If a filename starts with a '.', this character must be matched explicitly. (Thus, 'rm *' will not remove .profile,
 and 'tar c *' will not archive all your files; 'tar c .' is better.)

Note that wildcard patterns are not regular expressions, although they are a bit similar. First of all, they match
filenames, rather than text, and secondly, the conventions are not the same: for example, in a regular expression '*'
means zero or more copies of the preceding thing.

Now that regular expressions have bracket expressions where the negation is indicated by a '^', POSIX has declared the
effect of a wildcard pattern '[^...]' to be undefined.
*/

typedef bool (*bPFpcupcud)(const char*, uint32_t, const char*, uint32_t, int);

extern U_EXPORT int        u_pfn_flags;
extern U_EXPORT bPFpcupcud u_pfn_match;

extern U_EXPORT bool u_fnmatch(         const char* restrict s, uint32_t n1, const char* restrict pattern, uint32_t n2, int flags);
extern U_EXPORT bool u_dosmatch(        const char* restrict s, uint32_t n1, const char* restrict pattern, uint32_t n2, int flags) __pure;
extern U_EXPORT bool u_dosmatch_with_OR(const char* restrict s, uint32_t n1, const char* restrict pattern, uint32_t n2, int flags) __pure; /* multiple patterns separated by '|' */ 

enum MatchType { U_FNMATCH = 0, U_DOSMATCH = 1, U_DOSMATCH_WITH_OR = 2 };

#ifndef FNM_CASEFOLD
#define FNM_CASEFOLD    (1 <<  4)   /* Compare without regard to case */
#endif
#define FNM_INVERT      (1 << 28)   /* invert the result */

#ifndef FNM_IGNORECASE
#define FNM_IGNORECASE  FNM_CASEFOLD
#endif
#ifndef FNM_LEADING_DIR
#define FNM_LEADING_DIR FNM_PERIOD
#endif

static inline void u_setPfnMatch(int match_type, int flags)
{
   U_INTERNAL_TRACE("u_setPfnMatch(%d,%d)", match_type, flags)

   u_pfn_flags = flags;
   u_pfn_match = (match_type == U_FNMATCH  ? u_fnmatch  :
                  match_type == U_DOSMATCH ? u_dosmatch :
                                             u_dosmatch_with_OR);
}

extern U_EXPORT bool u_isURL(const char* restrict url, uint32_t len) __pure;

extern U_EXPORT bool u_isMacAddr(const char* restrict p, uint32_t len) __pure;

/* Change the current working directory to the `user` user's home dir, and downgrade security to that user account */

extern U_EXPORT bool u_runAsUser(const char* restrict user, bool change_dir);

/* Verifies that the passed string is actually an e-mail address */

extern U_EXPORT bool u_validate_email_address(const char* restrict address, uint32_t address_len) __pure;

/* Perform 'natural order' comparisons of strings. */

extern U_EXPORT int u_strnatcmp(char const* restrict a, char const* restrict b) __pure;

/* Get the number of the processors including offline CPUs */

extern U_EXPORT int u_get_num_cpu(void);

/* Pin the process to a particular core */

extern U_EXPORT void u_bind2cpu(pid_t pid, int n);

/** -------------------------------------------------------------------------------
// Canonicalize PATH, and build a new path. The new path differs from PATH in that:
// --------------------------------------------------------------------------------
// Multiple    '/'   are collapsed to a single '/'
// Leading     './'  are removed
// Trailing    '/.'  are removed
// Trailing    '/'   are removed
// Non-leading '../' and trailing '..' are handled by removing portions of the path
// -------------------------------------------------------------------------------- */

extern U_EXPORT bool u_canonicalize_pathname(char* restrict path);

/** --------------------------------------------------------------
// find a FILE MODE along PATH
// --------------------------------------------------------------
// pathfind looks for a a file with name FILENAME and MODE access
// along colon delimited PATH, and returns the full pathname as a
// string, or NULL if not found.
// -------------------------------------------------------------- */

extern U_EXPORT bool u_pathfind(char* restrict result, const char* restrict path, uint32_t path_len, const char* restrict filename, int mode); /* R_OK | X_OK */

/* Prepare command for call to exec() */

extern U_EXPORT uint32_t u_split(       char* restrict s, uint32_t n, char** restrict argv, const char* restrict delim);
extern U_EXPORT int      u_splitCommand(char* restrict s, uint32_t n, char** restrict argv, char* restrict pathbuf, uint32_t pathbuf_size);

/* FTW - walks through the directory tree starting from the indicated directory dir. For each found entry in
 *       the tree, it calls fn() with the full pathname of the entry, his length and if it is a directory
 */

struct u_ftw_ctx_s {
   qcompare sort_by;
   vPF call, call_if_up;
   uint32_t filter_len;
   const char* restrict filter;
   bool depth, is_directory, call_if_directory;

   /* NB: we use u_buffer...
    * -----------------------
    * uint32_t u_ftw_pathlen;
    * char*    u_ftw_fullpath;
    * -----------------------
    */
};

extern U_EXPORT   void u_ftw(void);
extern U_EXPORT struct u_ftw_ctx_s u_ftw_ctx;
extern U_EXPORT    int u_ftw_ino_cmp(const void* restrict a, const void* restrict b) __pure;

/* From RFC 3986 */

#define U_URI_UNRESERVED  0 /* ALPHA (%41-%5A and %61-%7A) DIGIT (%30-%39) '-' '.' '_' '~' */
#define U_URI_PCT_ENCODED 1
#define U_URI_GEN_DELIMS  2 /* ':' '/' '?' '#' '[' ']' '@' */
#define U_URI_SUB_DELIMS  4 /* '!' '$' '&' '\'' '(' ')' '*' '+' ',' ';' '=' */

extern U_EXPORT int                 u_uri_encoded_char_mask;
extern U_EXPORT const unsigned char u_uri_encoded_char[256];

/* character type identification - Assumed an ISO-1 character set */

extern U_EXPORT const unsigned int  u__ct_tab[256];
extern U_EXPORT const unsigned char u__ct_tol[256];
extern U_EXPORT const unsigned char u__ct_tou[256];

static inline bool u_iscntrl(int c)  { return ((u__ct_tab[c] & 0x001) != 0); } // __C } /* Control character. */
static inline bool u_isdigit(int c)  { return ((u__ct_tab[c] & 0x002) != 0); } // __D } /* Digit. */
static inline bool u_islower(int c)  { return ((u__ct_tab[c] & 0x004) != 0); } // __L } /* Lowercase. */
static inline bool u_ispunct(int c)  { return ((u__ct_tab[c] & 0x008) != 0); } // __P } /* Punctuation. */
static inline bool u_isspace(int c)  { return ((u__ct_tab[c] & 0x010) != 0); } // __S } /* Space. */
static inline bool u_isupper(int c)  { return ((u__ct_tab[c] & 0x020) != 0); } // __U } /* Uppercase. */
static inline bool u_isblank(int c)  { return ((u__ct_tab[c] & 0x080) != 0); } // __B } /* Blank (a space or a tab). */
static inline bool u_islterm(int c)  { return ((u__ct_tab[c] & 0x100) != 0); } // __R } /* carriage return or new line (a \r or \n). */
static inline bool u_istext( int c)  { return ((u__ct_tab[c] & 0x400) == 0); } // __F } /* character never appears in text */

static inline bool u_isalpha( int c) { return ((u__ct_tab[c] & 0x024) != 0); } // (__L | __U)
static inline bool u_isxdigit(int c) { return ((u__ct_tab[c] & 0x042) != 0); } // (__D | __X)
static inline bool u_isalnum( int c) { return ((u__ct_tab[c] & 0x026) != 0); } // (__D | __L | __U)
static inline bool u_isgraph( int c) { return ((u__ct_tab[c] & 0x02E) != 0); } // (__D | __L | __P | __U)
static inline bool u_isprint( int c) { return ((u__ct_tab[c] & 0x03E) != 0); } // (__D | __L | __P | __S | __U)

static inline int u_tolower(int c)   { return u__ct_tol[c]; }
static inline int u_toupper(int c)   { return u__ct_tou[c]; }

static inline bool u_isoctal( int c) { return ('0' <= c && c <= '7'); }
static inline char u_octc2int(int c) { return ((c - '0') & 07); }
static inline char u_hexc2int(int c) { return (u_isdigit(c) ? c - '0' : u_toupper(c) - 'A' + 10); }
static inline bool u_isbase64(int c) { return (u_isalnum(c) || (c == '+') || (c == '/') || (c == '=')); }

/* buffer type identification */

extern U_EXPORT bool u_isBase64(    const char* restrict s, uint32_t n) __pure;
extern U_EXPORT bool u_isWhiteSpace(const char* restrict s, uint32_t n) __pure;

enum TextType {
   U_TYPE_TEXT_ASCII, /* X3.4, ISO-8859, non-ISO ext. ASCII */
   U_TYPE_TEXT_UTF8,
   U_TYPE_TEXT_UTF16LE,
   U_TYPE_TEXT_UTF16BE,
   U_TYPE_BINARY_DATA
};

extern U_EXPORT bool u_isText(  const unsigned char* restrict s, uint32_t n) __pure;
extern U_EXPORT bool u_isUTF8(  const unsigned char* restrict s, uint32_t n) __pure;
extern U_EXPORT int  u_isUTF16( const unsigned char* restrict s, uint32_t n) __pure;
extern U_EXPORT bool u_isBinary(const unsigned char* restrict s, uint32_t n) __pure;

/* ip address type identification */

static inline bool u_isIPv4Addr(const char* restrict p, uint32_t n)
   { char c; uint32_t i = 0; for (; i < n; ++i) { c = p[i]; if (u_isdigit(c)  == 0 && c != '.')             return false; } return true; } 

static inline bool u_isIPv6Addr(const char* restrict p, uint32_t n)
   { char c; uint32_t i = 0; for (; i < n; ++i) { c = p[i]; if (u_isxdigit(c) == 0 && c != '.' && c != ':') return false; } return true; } 

static inline bool u_isIPAddr(bool IPv6, const char* restrict p, uint32_t n) { return (IPv6 ? u_isIPv6Addr(p, n) : u_isIPv4Addr(p, n)); } 

/* Quick and dirty int->hex. The only standard way is to call snprintf (?),
 * which is undesirably slow for such a frequently-called function... */

static inline void u_int2hex(char* restrict p, uint32_t n)
   { int s; for (s = 28; s >= 0; s -= 4, ++p) *p = u_hex_upper[((n >> s) & 0x0F)]; }

static inline uint32_t u_hex2int(const char* restrict p, uint32_t len)
   { uint32_t n = 0; const char* eos = p + len; while (p < eos) n = (n << 4) | u_hexc2int(*p++); return n; }

#ifdef HAVE_SSL
/*
The u_passwd_cb() function must write the password into the provided buffer buf which is of size size.
The actual length of the password must be returned to the calling function. rwflag indicates whether the
callback is used for reading/decryption (rwflag=0) or writing/encryption (rwflag=1).
See man SSL_CTX_set_default_passwd_cb(3) for more information
*/
extern U_EXPORT int u_passwd_cb(char* restrict buf, int size, int rwflag, void* restrict password);
#endif

#ifdef __cplusplus
}
#endif

#endif
