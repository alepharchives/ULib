/** ============================================================================
//
// = LIBRARY
//    ulibase - c library
//
// = FILENAME
//    macro.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================ */

#ifndef ULIBASE_MACRO_H
#define ULIBASE_MACRO_H 1

/* Manage location info */

#define U_SET_LOCATION_INFO (u_num_line      = __LINE__, \
                             u_name_file     = __FILE__, \
                             u_name_function = __PRETTY_FUNCTION__)

/* Manage debug for C library */

#ifdef DEBUG_DEBUG
#  define U_INTERNAL_TRACE(format,args...) \
      { char u_internal_buf[8192]; (void) sprintf(u_internal_buf, format"\n", args); \
        (void) write(STDERR_FILENO, u_internal_buf, strlen(u_internal_buf)); }
#  define U_INTERNAL_PRINT(format,args...) U_INTERNAL_TRACE(format,args)
#else
#  define U_INTERNAL_TRACE(format,args...)
#  define U_INTERNAL_PRINT(format,args...)
#endif

/* Design by contract - if (assertion == false) then stop */

#define U_ASSERT_MACRO(assertion,msg,info) \
      if ((bool)(assertion) == false) { \
         u_printf("%W%N%W: %Q%W%s%W\n" \
         "-------------------------------------\n" \
         " pid: %W%P%W\n" \
         " file: %W%s%W\n" \
         " line: %W%d%W\n" \
         " function: %W%s%W\n" \
         " assertion: \"(%W%s%W)\" %W%s%W\n" \
         "-------------------------------------", \
         GREEN, YELLOW, -1, CYAN, msg, YELLOW, \
         CYAN, YELLOW, \
         CYAN, __FILE__, YELLOW, \
         CYAN, __LINE__, YELLOW, \
         CYAN, __PRETTY_FUNCTION__, YELLOW, \
         CYAN, #assertion, YELLOW, MAGENTA, info, YELLOW); }

#ifdef DEBUG
#  define U_INTERNAL_ASSERT(expr)        { U_ASSERT_MACRO(expr,"ASSERTION FALSE","") }
#  define U_INTERNAL_ASSERT_MINOR(a,b)   { U_ASSERT_MACRO((a)<(b),"NOT LESS","") }
#  define U_INTERNAL_ASSERT_MAJOR(a,b)   { U_ASSERT_MACRO((a)>(b),"NOT GREATER","") }
#  define U_INTERNAL_ASSERT_EQUALS(a,b)  { U_ASSERT_MACRO((a)==(b),"NOT EQUALS","") }
#  define U_INTERNAL_ASSERT_DIFFERS(a,b) { U_ASSERT_MACRO((a)!=(b),"NOT DIFFERENT","") }
#  define U_INTERNAL_ASSERT_POINTER(ptr) { U_ASSERT_MACRO((void*)ptr>(void*)0x0000ffff,"~NULL POINTER","") }
#  define U_INTERNAL_ASSERT_RANGE(a,x,b) { U_ASSERT_MACRO((x)>=(a)&&(x)<=(b),"VALUE OUT OF RANGE","") }

#  define U_INTERNAL_ASSERT_MSG(expr,info) \
          { U_ASSERT_MACRO(expr,"ASSERTION FALSE",info) }
#  define U_INTERNAL_ASSERT_MINOR_MSG(a,b,info) \
          { U_ASSERT_MACRO((a)<(b),"NOT LESS",info) }
#  define U_INTERNAL_ASSERT_MAJOR_MSG(a,b,info) \
          { U_ASSERT_MACRO((a)>(b),"NOT GREATER",info) }
#  define U_INTERNAL_ASSERT_EQUALS_MSG(a,b,info) \
          { U_ASSERT_MACRO((a)==(b),"NOT EQUALS",info) }
#  define U_INTERNAL_ASSERT_DIFFERS_MSG(a,b,info) \
          { U_ASSERT_MACRO((a)!=(b),"NOT DIFFERENT",info) }
#  define U_INTERNAL_ASSERT_POINTER_MSG(ptr,info) \
          { U_ASSERT_MACRO((void*)ptr>(void*)0x0000ffff,"~NULL POINTER",info) }
#  define U_INTERNAL_ASSERT_RANGE_MSG(a,x,b,info) \
          { U_ASSERT_MACRO((x)>=(a)&&(x)<=(b),"VALUE OUT OF RANGE",info) }
#else
#  define U_INTERNAL_ASSERT(expr)
#  define U_INTERNAL_ASSERT_MINOR(a,b)
#  define U_INTERNAL_ASSERT_MAJOR(a,b)
#  define U_INTERNAL_ASSERT_EQUALS(a,b)
#  define U_INTERNAL_ASSERT_DIFFERS(a,b)
#  define U_INTERNAL_ASSERT_POINTER(ptr)
#  define U_INTERNAL_ASSERT_RANGE(a,x,b)

#  define U_INTERNAL_ASSERT_MSG(expr,info)
#  define U_INTERNAL_ASSERT_MINOR_MSG(a,b,info)
#  define U_INTERNAL_ASSERT_MAJOR_MSG(a,b,info)
#  define U_INTERNAL_ASSERT_EQUALS_MSG(a,b,info)
#  define U_INTERNAL_ASSERT_DIFFERS_MSG(a,b,info)
#  define U_INTERNAL_ASSERT_POINTER_MSG(ptr,info)
#  define U_INTERNAL_ASSERT_RANGE_MSG(a,x,b,info)
#endif

/* Manage message info */

#  define U_ERROR(  format,args...) u_printf("%W%N%W: %Q%WERROR: "format"%W",BRIGHTCYAN,YELLOW, 1,RED,args,RESET)
#  define U_ABORT(  format,args...) u_printf("%W%N%W: %Q%WABORT: "format"%W",BRIGHTCYAN,YELLOW,-1,RED,args,RESET)
#  define U_WARNING(format,args...) u_printf("%W%N%W: WARNING: "  format"%W",BRIGHTCYAN,YELLOW,       args,RESET)
#  define U_MESSAGE(format,args...) u_printf("%W%N%W: "           format,    BRIGHTCYAN,RESET,        args)

#  define   U_ERROR_SYSCALL(msg)      U_ERROR("%R",msg)
#  define   U_ABORT_SYSCALL(msg)      U_ABORT("%R",msg)
#  define U_WARNING_SYSCALL(msg)    U_WARNING("%R",msg)

/* get string costant size from compiler */

#define U_CONSTANT_SIZE(str)     (sizeof(str)-1)
#define U_CONSTANT_TO_PARAM(str) str,U_CONSTANT_SIZE(str)
#define U_CONSTANT_TO_TRACE(str)     U_CONSTANT_SIZE(str),str

#define U_STRNCPY(a,b)    (void)strncpy((const char*)(a),b,U_CONSTANT_SIZE(b))
#define U_STRNCMP(a,b)          strncmp((const char*)(a),b,U_CONSTANT_SIZE(b))
#define U_STRNCASECMP(a,b)  strncasecmp((const char*)(a),b,U_CONSTANT_SIZE(b))

#define U_STREQ(a,b)  (strcmp( (const char*)(a),b) == 0)
#define U_STRNEQ(a,b) (U_STRNCMP((a),b)            == 0)

#define U_MEMCPY(a,b) memcpy((      char*)(a),b,U_CONSTANT_SIZE(b))
#define U_MEMCMP(a,b) memcmp((const char*)(a),b,U_CONSTANT_SIZE(b))

/* Note that IS_ABSOLUTE_PATH accepts d:foo as well, although it is only semi-absolute. This is because the users of IS_ABSOLUTE_PATH
 * want to know whether to prepend the current working directory to a file name, which should not be done with a name like d:foo
 */

#ifdef __MINGW32__
#  define PATH_SEPARATOR        '\\'
#  define IS_DIR_SEPARATOR(c)   ((c) == '/' || (c) == '\\')
#  define IS_ABSOLUTE_PATH(f)   (IS_DIR_SEPARATOR((f)[0]) || (((f)[0]) && ((f)[1] == ':')))
#  define FILENAME_CMP(s1, s2)  strcasecmp(s1, s2)
#  define U_PATH_CONV(s)        u_slashify(s, '/', '\\')
#  define U_PATH_SHELL          U_CONSTANT_TO_PARAM("sh.exe ")
#else
#  define PATH_SEPARATOR        '/'
#  define IS_DIR_SEPARATOR(c)   ((c) == '/')
#  define IS_ABSOLUTE_PATH(f)   (IS_DIR_SEPARATOR((f)[0]))
#  define FILENAME_CMP(s1, s2)  strcmp(s1, s2)
#  define U_PATH_CONV(s)        (s)
#  define U_PATH_SHELL          U_CONSTANT_TO_PARAM("/bin/sh ")
#endif

/* Defs */

#ifndef U_min
#  define U_min(x,y) ((x) <= (y) ? (x) : (y))
#endif
#ifndef U_max
#  define U_max(x,y) ((x) >= (y) ? (x) : (y))
#endif

#ifndef   O_BINARY
#  define O_BINARY 0 /* unix is binary by default */
#endif

#ifndef   U_PATH_MAX
#  define U_PATH_MAX 1024U
#endif

#ifndef   MAX_FILENAME_LEN
#  define MAX_FILENAME_LEN 255U
#endif

#ifndef   PAGESIZE
#  define PAGESIZE 4096U
#endif

#ifndef   U_ONE_DAY_IN_SECOND
#  define U_ONE_DAY_IN_SECOND (24 * 60 * 60)
#endif

#define U_NOT_FOUND ((uint32_t)-1)

#define GZIP_MAGIC "\037\213" /* Magic header for gzip files, 1F 8B */

#ifdef  NULL
#undef  NULL
#endif
#define NULL ((void*)0)

enum AffermationType { U_MAYBE = 0, U_YES = 1, U_NOT = 2 };

/**
 * Enumeration of Hash (Digest) types
 * 
 * The hash types known to openssl
 **/

typedef enum {
   U_HASH_MD2       = 0,
   U_HASH_MD5       = 1,
   U_HASH_SHA       = 2,
   U_HASH_SHA1      = 3,
   U_HASH_SHA224    = 4,
   U_HASH_SHA256    = 5,
   U_HASH_SHA384    = 6,
   U_HASH_SHA512    = 7,
   U_HASH_MDC2      = 8,
   U_HASH_RIPEMD160 = 9
} UHashType;

#if SIZEOF_VOID_P == 4
#  define int2ptr(x) ((void*)(uint32_t)x)
#  define ptr2int(x) ((uint64_t)(uint32_t)x)
#else
#  define int2ptr(x) ((void*)x)
#  define ptr2int(x) ((uint64_t)x)
#endif

/* Type of file sizes and offsets (LFS) */

#if SIZEOF_OFF_T == SIZEOF_LONG
#  define u_strtooff(nptr,endptr,base) (off_t) strtol((nptr),(char**)(endptr),(base))
#else
#  define u_strtooff(nptr,endptr,base) (off_t) strtoll((nptr),(char**)(endptr),(base))

#  if SIZEOF_OFF_T != 8
#     error ERROR: unexpected size of SIZEOF_OFF_T
#  endif
#endif

/* Line terminator */

#define U_LF    "\n"
#define U_LF2   "\n\n"
#define U_CRLF  "\r\n"
#define U_CRLF2 "\r\n\r\n"

#define U_CONCAT2(a,b)   a##b
#define U_CONCAT3(a,b,c) a##b##c

/* Shared library support */

/* Visibility is available for GCC newer than 3.4.
 See: http://gcc.gnu.org/bugzilla/show_bug.cgi?id=9283

 The U_NO_EXPORT macro marks the symbol of the given variable to be hidden.
 A hidden symbol is stripped during the linking step, so it can't be used
 from outside the resulting library, which is similar to static. However,
 static limits the visibility to the current compilation unit. hidden symbols
 can still be used in multiple compilation units.
 \code
 int U_EXPORT bar;
 int U_NO_EXPORT foo;
 \end
*/

#define U_EXPORT
#define U_NO_EXPORT

#ifdef __MINGW32__
#  undef  U_EXPORT
#  define U_EXPORT __declspec(dllexport)
#elif defined(LINUX) || defined(__LINUX__) || defined(__linux__)
#  if defined(HAVE_GNUC_VISIBILTY)
#     undef  U_EXPORT
#     define U_EXPORT    __attribute__ ((visibility("default")))
#     undef  U_NO_EXPORT
#     define U_NO_EXPORT __attribute__ ((visibility("hidden")))
#  endif
#endif

/* Provide convenience macros for handling structure fields through their offsets */

#define U_STRUCT_MEMBER_SIZE(type,member)   (sizeof(((type*)0)->member))
#define U_STRUCT_MEMBER_OFFSET(type,member)       (&((type*)0)->member)

#define U_STRUCT_IS_LAST_MEMBER(type,member)    (U_STRUCT_MEMBER_END_OFFSET(type,member) == sizeof(type))
#define U_STRUCT_MEMBER_END_OFFSET(type,member) (U_STRUCT_MEMBER_SIZE(type,member) + U_STRUCT_OFFSET(type,member))

/* Needed for unaligned memory access */

#ifdef HAVE_NO_UNALIGNED_ACCESSES

struct __una_u32 { uint32_t x __attribute__((packed)); };

#  define u_get_unalignedp(ptr)      ((uint32_t)(((const struct __una_u32*)(ptr))->x))
#  define u_put_unalignedp(val, ptr)             ((      struct __una_u32*)(ptr))->x = val

#else

/* The x86 can do unaligned accesses itself */

#  define u_get_unalignedp(ptr)      (*(uint32_t*)(ptr))
#  define u_put_unalignedp(val, ptr) (*(uint32_t*)(ptr) = (val))

#endif

/**
 * u_get_unaligned - get value from possibly mis-aligned location
 *
 * This macro should be used for accessing values larger in size than
 * single bytes at locations that are expected to be improperly aligned
 *
 * Note that unaligned accesses can be very expensive on some architectures.
 */

#define u_get_unaligned(ref) u_get_unalignedp(&(ref))

/**
 * u_put_unaligned - put value to a possibly mis-aligned location
 *
 * This macro should be used for placing values larger in size than
 * single bytes at locations that are expected to be improperly aligned
 *
 * Note that unaligned accesses can be very expensive on some architectures.
 */

#define u_put_unaligned(val, ref) u_put_unalignedp(val,&(ref))

/* Endian order */

#if __BYTE_ORDER == __LITTLE_ENDIAN
#  define u_test_bit(n,c) (((c) & (1L << n)) != 0)

#  define u_byte4(n) ( (n) << 24)
#  define u_byte3(n) (((n) << 8) & 0x00ff0000)
#  define u_byte2(n) (((n) >> 8) & 0x0000ff00)
#  define u_byte1(n) ( (n) >> 24)

#else
#  define u_test_bit(n,c) ((((c) >> n) & 1L) != 0)

#  define u_byte4(n) ( (n) >> 24)
#  define u_byte3(n) (((n) >> 8) & 0x0000ff00)
#  define u_byte2(n) (((n) << 8) & 0x00ff0000)
#  define u_byte1(n) ( (n) << 24)

#endif

#define u_invert_uint16(s) (((s) >> 8) | \
                            ((s) << 8))

#define u_invert_uint32(n) (u_byte4(n) | \
                            u_byte3(n) | \
                            u_byte2(n) | \
                            u_byte1(n))

/* Needed for C language */

/* Check for dot entry in directory */

#define U_ISDOTS(d_name) (d_name[0] == '.' && (d_name[1] == '\0' || (d_name[1] == '.' && d_name[2] == '\0')))

/* Memory alignment for pointer */

#define U_MEMORY_ALIGNMENT(ptr, alignment)  ptr += alignment - ((long)ptr & (alignment - 1))

/* Manage number suffix */

#define U_NUMBER_SUFFIX(number,suffix) \
   switch (suffix) { \
      case 'G': number <<= 10; \
      case 'M': number <<= 10; \
      case 'K': \
      case 'k': number <<= 10; \
      break; }

/* skip white space */

#define U_SKIP(s,end,loop,done) \
                                \
   if (s >= end)                \
      {                         \
      goto done;                \
      }                         \
                                \
   if (u_isspace(*s))           \
      {                         \
      ++s;                      \
                                \
      goto loop;                \
      }

/* skip char delimiter */

#define U_SKIP_CHAR(s,end,loop,done,c) \
                                       \
   if (s >= end)                       \
      {                                \
      goto done;                       \
      }                                \
                                       \
   if (*s == c)                        \
      {                                \
      ++s;                             \
                                       \
      goto loop;                       \
      }

/* skip string delimiter */

#define U_SKIP_EXT(s,end,loop,done,delim) \
                                          \
   if (s >= end)                          \
      {                                   \
      goto done;                          \
      }                                   \
                                          \
   if (strchr(delim, *s))                 \
      {                                   \
      ++s;                                \
                                          \
      goto loop;                          \
      }

/* skip line comment */

#define U_SKIP_LINE_COMMENT(s,end,loop)       \
                                              \
      {                                       \
      s = (char*) memchr(s, '\n', end - s);   \
                                              \
      if (s == 0) s = end;                    \
                                              \
      goto loop;                              \
      }

/* skip quoted string */

#define U_SKIP_QUOTED_STRING(s,end,q)         \
                                              \
      {                                       \
      while (true)                            \
         {                                    \
         s = (char*) memchr(s, q, end - s);   \
                                              \
         if (s == 0) s = end;                 \
         else                                 \
            {                                 \
            if (*(s-1) == '\\')               \
               {                              \
               ++s;                           \
                                              \
               continue;                      \
               }                              \
            }                                 \
                                              \
         break;                               \
         }                                    \
      }

/* delimit token with char delimiter */

#define U_DELIMIT_TOKEN_CHAR(s,end,p,c) \
                                        \
   p = s;                               \
                                        \
   s = (char*) memchr(s, c, end - s);   \
                                        \
   if (s == 0) s = end;

/* delimit token with white space */

#define U_DELIMIT_TOKEN(s,end,p)        \
                                        \
   if (*s == '"')                       \
      {                                 \
      p = ++s;                          \
                                        \
      U_SKIP_QUOTED_STRING(s,end,'"')   \
      }                                 \
   else                                 \
      {                                 \
      p = s++;                          \
                                        \
      while (s < end &&                 \
             u_isspace(*s) == false)    \
         {                              \
         ++s;                           \
         }                              \
      }

#define U_DELIMIT_TOKENC(c,s,end,p)     \
                                        \
   c = *s;                              \
                                        \
   if (c == '"'  ||                     \
       c == '\'')                       \
      {                                 \
      p = ++s;                          \
                                        \
      U_SKIP_QUOTED_STRING(s,end,c)     \
      }                                 \
   else                                 \
      {                                 \
      p = s++;                          \
                                        \
      while (s < end &&                 \
             u_isspace(*s) == false)    \
         {                              \
         ++s;                           \
         }                              \
      }

/* delimit token with string delimiter */

#define U_DELIMIT_TOKEN_EXT(s,end,p,delim)      \
                                                \
   if (*s == '"')                               \
      {                                         \
      p = s++;                                  \
                                                \
      U_SKIP_QUOTED_STRING(s,end,'"')           \
                                                \
      if (s < end) ++s;                         \
      }                                         \
   else                                         \
      {                                         \
      p = s++;                                  \
                                                \
      s = (char*) u_strpbrk(s, end - s, delim); \
                                                \
      if (s == 0) s = end;                      \
      }

#endif
