// ============================================================================
//
// = LIBRARY
//    ulibase - c library
//
// = FILENAME
//    base.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================ */

#ifndef ULIBASE_BASE_H
#define ULIBASE_BASE_H 1

/* Manage file to include */

#include <ulib/base/top.h>

#ifdef HAVE_CONFIG_H
#  include <ulib/internal/config.h>
#else
#  define HAVE_STRERROR
#  define HAVE_STRSIGNAL
#endif

#include <ulib/base/bottom.h>

/* Defs */

#include <ulib/base/color.h>
#include <ulib/base/macro.h>

#include <stddef.h>

/*
#ifdef  restrict
#undef  restrict
#endif
#define restrict
*/

#ifdef __cplusplus
extern "C" {
#endif

typedef void  (*vPFi)    (int);
typedef bool  (*bPFi)    (int);
typedef void  (*vPF)     (void);
typedef void* (*pvPF)    (void);
typedef void  (*vPFpv)   (void*);
typedef int   (*iPFpv)   (void*);
typedef bool  (*bPF)     (void);
typedef bool  (*bPFpv)   (void*);
typedef void  (*vPFpvpv) (void*,void*);
typedef bool  (*bPFpvpv) (void*,void*);
typedef bool  (*bPFpcpv) (const char*, const void*);
typedef int   (*qcompare)(const void*, const void*);

/* Startup */
extern U_EXPORT bool                 u_is_tty;
extern U_EXPORT pid_t                u_pid;
extern U_EXPORT uint32_t             u_pid_str_len;
extern U_EXPORT uint32_t             u_progname_len;
extern U_EXPORT       char* restrict u_pid_str;
extern U_EXPORT const char* restrict u_progpath;
extern U_EXPORT const char* restrict u_progname;

U_EXPORT void u_init_ulib(char** restrict argv);

/* AT EXIT */
extern U_EXPORT vPF u_fns[32];
extern U_EXPORT int u_fns_index;

U_EXPORT void u_exit(void);
U_EXPORT void u_atexit(vPF function);
U_EXPORT void u_unatexit(vPF function);

/* Current working directory */
extern U_EXPORT char*    u_cwd;
extern U_EXPORT uint32_t u_cwd_len;

U_EXPORT void u_getcwd(void);

/* Location info */
extern U_EXPORT uint32_t             u_num_line;
extern U_EXPORT const char* restrict u_name_file;
extern U_EXPORT const char* restrict u_name_function;

/* Temporary buffer */
extern U_EXPORT char*    u_buffer;
extern U_EXPORT uint32_t u_buffer_len; /* assert that is busy if != 0 */

/* Time services */
extern U_EXPORT bool   u_daylight;
extern U_EXPORT void*  u_pthread_time; /* pthread clock */
extern U_EXPORT time_t u_now_adjust;   /* GMT based time */
extern U_EXPORT time_t u_start_time;

extern U_EXPORT const char* u_months[];    /* "jan", "feb", "mar", "apr", "may", "jun", "jul", "aug", "sep", "oct", "nov", "dec" */
extern U_EXPORT const char* u_months_it[]; /* "gen", "feb", "mar", "apr", "mag", "giu", "lug", "ago", "set", "ott", "nov", "dic" */

extern U_EXPORT struct tm       u_strftime_tm;
extern U_EXPORT struct timeval* u_now;
extern U_EXPORT struct timeval  u_timeval;

/* Scan services */
extern U_EXPORT uint32_t             u_line_terminator_len;
extern U_EXPORT const char* restrict u_line_terminator;

/* Services */
extern U_EXPORT int                   u_errno; /* An errno value */
extern U_EXPORT int                   u_flag_exit;
extern U_EXPORT int                   u_flag_test;
extern U_EXPORT bool                  u_recursion;
extern U_EXPORT int                   u_mime_index;
extern U_EXPORT bool                  u_exec_failed;
extern U_EXPORT char                  u_hostname[255];
extern U_EXPORT char                  u_user_name[32];
extern U_EXPORT uint32_t              u_hostname_len, u_user_name_len;
extern U_EXPORT const unsigned char   u_alphabet[];  /* "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/" */
extern U_EXPORT const unsigned char   u_hex_upper[]; /* "0123456789ABCDEF" */
extern U_EXPORT const unsigned char   u_hex_lower[]; /* "0123456789abcdef" */
extern U_EXPORT const char* restrict  u_tmpdir;

U_EXPORT void        u_setPid(void);
U_EXPORT bool        u_setStartTime(void);
U_EXPORT void        u_init_ulib_username(void);
U_EXPORT void        u_init_ulib_hostname(void);
U_EXPORT int         u_getMonth(const char* buf) __pure;
U_EXPORT const char* u_basename(const char* restrict path) __pure;
U_EXPORT const char* u_getsuffix(const char* restrict path, uint32_t len) __pure;

U_EXPORT uint32_t u__strftime(char* restrict buffer, uint32_t buffer_size, const char* restrict fmt);
U_EXPORT uint32_t u_strftime( char* restrict buffer, uint32_t buffer_size, const char* restrict fmt, time_t when);

/* mime type identification */

static inline bool u_is_gz(void)   { return (u_mime_index == U_gz); }
static inline bool u_is_js(void)   { return (u_mime_index == U_js); }
static inline bool u_is_css(void)  { return (u_mime_index == U_css); }
static inline bool u_is_cgi(void)  { return (u_mime_index == U_cgi); }
static inline bool u_is_usp(void)  { return (u_mime_index == U_usp); }
static inline bool u_is_ssi(void)  { return (u_mime_index == U_ssi); }
static inline bool u_is_flv(void)  { return (u_mime_index == U_flv); }
static inline bool u_is_gif(void)  { return (u_mime_index == U_gif); }
static inline bool u_is_jpg(void)  { return (u_mime_index == U_jpg); }
static inline bool u_is_png(void)  { return (u_mime_index == U_png); }
static inline bool u_is_img(void)  { return (u_mime_index == U_png || u_mime_index == U_gif || u_mime_index == U_jpg || u_mime_index == U_ico); }
static inline bool u_is_html(void) { return (u_mime_index == U_html); }

/* Print with format extension: bBCDHMNOPQrRSUYwW
---------------------------------------------------------------------------------
'%b': print bool ("true" or "false")
'%B': print bit conversion of integer
'%C': print formatted char
'%H': print name host
'%M': print memory dump
'%N': print name program
'%P': print pid process
'%Q': sign for call to exit() or abort() (var-argument is param to exit)
'%r': print u_getExitStatus(exit_value)
'%R': print var-argument (msg) "-" u_getSysError()
'%S': print formatted string
'%O': print formatted temporary string + free(string)
'%U': print name login user
'%Y': print u_getSysSignal(signo)
'%w': print current working directory
'%W': print COLOR (index to ANSI ESCAPE STR)
---------------------------------------------------
'%D': print date and time in various format

with flag '#' => var-argument
---------------------------------------------------------------------------------
            0  => format: %d/%m/%y
with flag  '1' => format:          %T (=> "%H:%M:%S)
with flag  '2' => format:          %T (=> "%H:%M:%S) +n days
with flag  '3' => format: %d/%m/%Y %T
with flag  '4' => format: %d%m%y_%H%M%S_millisec (for file name, backup, etc...)
with flag  '5' => format: %a, %d %b %Y %H:%M:%S %Z
with flag  '6' => format: %Y/%m/%d %T
with flag  '9' => format: %d/%m/%y %T
---------------------------------------------------------------------------------
with flag '10' => format: %d/%m/%y %T
with flag '11' => format: %d/%b/%Y:%H:%M:%S %z
with flag '12' => format: %a, %d %b %Y %H:%M:%S GMT
---------------------------------------------------------------------------------
*/
extern U_EXPORT int     u_printf_fileno;
extern U_EXPORT int32_t u_printf_string_max_length;

/* NB: u_printf(), u_vsnprintf and u_snprintf conflit with /usr/include/unicode/urename.h */

U_EXPORT void     u__printf(                                                const char* restrict format, ...);
U_EXPORT void     u_printf2(int fd,                                         const char* restrict format, ...);
U_EXPORT void     u_internal_print(bool abrt,                               const char* restrict format, ...);
U_EXPORT uint32_t u__snprintf( char* restrict buffer, uint32_t buffer_size, const char* restrict format, ...);
U_EXPORT uint32_t u__vsnprintf(char* restrict buffer, uint32_t buffer_size, const char* restrict format, va_list argp);

#ifdef __cplusplus
}
#endif

#endif
