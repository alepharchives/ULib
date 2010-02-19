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

#ifdef __cplusplus
extern "C" {
#endif

typedef void  (*vPF)     (void);
typedef void* (*pvPF)    (void);
typedef void  (*vPFpv)   (void*);
typedef void  (*vPFpvpv) (void*,void*);
typedef bool  (*bPF)     (void);
typedef bool  (*bPFpv)   (void*);
typedef bool  (*bPFpcpv) (const char*, const void*);
typedef int   (*qcompare)(const void*, const void*);

/* Startup */
extern U_EXPORT bool     u_is_stderr_tty;
extern U_EXPORT pid_t    u_pid;
extern U_EXPORT char*    u_pid_str;
extern U_EXPORT uint32_t u_pid_str_len;
extern U_EXPORT char*    u_progpath;
extern U_EXPORT char*    u_progname;
extern U_EXPORT uint32_t u_progname_len;

extern U_EXPORT void u_init(char** argv);

/* AT EXIT */

extern U_EXPORT vPF u_fns[32];
extern U_EXPORT int u_fns_index;

extern U_EXPORT void u_exit(void);
extern U_EXPORT void u_atexit(vPF function);
extern U_EXPORT void u_unatexit(vPF function);

/* Current working directory */
extern U_EXPORT char        u_cwd_buffer[256];
extern U_EXPORT uint32_t    u_cwd_len;
extern U_EXPORT const char* u_cwd;

extern U_EXPORT void u_getcwd(void);

/* Location info */
extern U_EXPORT uint32_t    u_num_line;
extern U_EXPORT const char* u_name_file;
extern U_EXPORT const char* u_name_function;

/* Temporary buffer: for example is used by u_ftw */
extern U_EXPORT char     u_buffer[4096];
extern U_EXPORT uint32_t u_buffer_len; /* assert that is busy */

/* Time services */
extern U_EXPORT time_t u_start_time;
extern U_EXPORT time_t u_now_adjust; /* GMT based time */
extern U_EXPORT struct timeval u_now;
extern U_EXPORT struct tm u_strftime_tm;

/* Scan services */
extern U_EXPORT const char* u_line_terminator;
extern U_EXPORT uint32_t    u_line_terminator_len;

/* Services */
extern U_EXPORT int                 u_errno;
extern U_EXPORT bPF                 u_at_exit;
extern U_EXPORT int                 u_flag_exit;
extern U_EXPORT int                 u_flag_test;
extern U_EXPORT bool                u_recursion;
extern U_EXPORT bool                u_exec_failed;
extern U_EXPORT char                u_hostname[255];
extern U_EXPORT char                u_user_name[32];
extern U_EXPORT uint32_t            u_hostname_len, u_user_name_len;
extern U_EXPORT const char*         u_tmpdir;
extern U_EXPORT const unsigned char u_alphabet[64];  /* "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/" */
extern U_EXPORT const unsigned char u_hex_upper[16]; /* "0123456789ABCDEF" */
extern U_EXPORT const unsigned char u_hex_lower[16]; /* "0123456789abcdef" */

extern U_EXPORT void        u_setPid(void);
extern U_EXPORT const char* u_basename(const char* path);
extern U_EXPORT uint32_t    u_strftime(char* buffer, uint32_t buffer_size, const char* fmt, time_t now);

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

'%D': print date and time in various format

with flag '#' => var-argument
with flag '1' => format: %d/%m/%y
with flag '2' => format:          %T (=> "%H:%M:%S)
with flag '3' => format:          %T (=> "%H:%M:%S) +n days
with flag '4' => format: %d/%m/%y %T
with flag '5' => format: %d/%m/%Y %T
with flag '6' => format: %d%m%y_%H%M%S_millisec (for file name, backup, etc...)
with flag '7' => format: %a, %d %b %Y %H:%M:%S     (HTTP header)

         default format: %a, %d %b %Y %H:%M:%S GMT (HTTP header) (use u_now)
---------------------------------------------------------------------------------
*/
extern U_EXPORT int32_t u_printf_string_max_length; /* default = 128 */

extern U_EXPORT void        u_printf(                                    const char* format, ...);
extern U_EXPORT uint32_t  u_snprintf(char* buffer, uint32_t buffer_size, const char* format, ...);
extern U_EXPORT uint32_t u_vsnprintf(char* buffer, uint32_t buffer_size, const char* format, va_list argp);

#ifdef __cplusplus
}
#endif

#endif
