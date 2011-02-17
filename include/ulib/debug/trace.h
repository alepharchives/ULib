// ============================================================================
//
// = LIBRARY
//    ulibdbg - c++ library
//
// = FILENAME
//    trace.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIBDBG_TRACE_H
#define ULIBDBG_TRACE_H 1

#include <ulib/base/trace.h>

#include <ulib/debug/macro.h>
#include <ulib/debug/error_simulation.h>

#define U_MANAGE_RETURN_VALUE(type,format) \
   type trace_return_type(type ret) { \
   void* ptr_value = USimulationError::checkForMatch(buffer_trace); \
   if (ptr_value) \
      ret = *(type*)ptr_value; \
   trace_return(format, ret); \
   return ret; }

#define U_MANAGE_SYSRETURN_VALUE(type,format,error) \
   type trace_sysreturn_type(type ret) { \
   void* ptr_value = USimulationError::checkForMatch(buffer_syscall); \
   if (ptr_value) \
      ret = *(type*)ptr_value; \
   trace_sysreturn((error), format, ret); \
   return ret; }

typedef                void* pvoid_t;
typedef                char* pchar_t;
typedef const          char* pcchar_t;
typedef       unsigned char* puchar_t;
typedef const unsigned char* pcuchar_t;

// typedef int (*x11error_t)  (void*, void*);
// typedef int (*x11IOerror_t)(void*);

class U_EXPORT UTrace {
public:

   // Initialization and termination methods.

    UTrace(int level, const char* format, ...);
   ~UTrace();

   // trace return from generic call

   void trace_return(const char* format, ...);

   // manage return from generic call for tipology of value (of return)...

   U_MANAGE_RETURN_VALUE(bool,               "%b")
   U_MANAGE_RETURN_VALUE(char,               "%C")
   U_MANAGE_RETURN_VALUE(int,                "%d")
   U_MANAGE_RETURN_VALUE(unsigned int,       "%u")
   U_MANAGE_RETURN_VALUE(long,               "%ld")
   U_MANAGE_RETURN_VALUE(unsigned long,      "%lu")
   U_MANAGE_RETURN_VALUE(long long,          "%lld")
   U_MANAGE_RETURN_VALUE(unsigned long long, "%llu")
   U_MANAGE_RETURN_VALUE(float,              "%f")
   U_MANAGE_RETURN_VALUE(double,             "%g")
   U_MANAGE_RETURN_VALUE(long double,        "%LG")
   U_MANAGE_RETURN_VALUE(void*,              "%p")
   U_MANAGE_RETURN_VALUE(const void*,        "%p")
   U_MANAGE_RETURN_VALUE(char*,              "%S")
   U_MANAGE_RETURN_VALUE(const char*,        "%S")
   U_MANAGE_RETURN_VALUE(void**,             "%p")
   U_MANAGE_RETURN_VALUE(char**,             "%p")

   // trace call and return from system call

   void trace_syscall(              const char* format, ...);
   void trace_sysreturn(bool error, const char* format, ...);

   // manage return from system call for tipology of value (of return)...

   U_MANAGE_SYSRETURN_VALUE(int,                "%d",   ret == -1)
   U_MANAGE_SYSRETURN_VALUE(unsigned int,       "%u",   ret == 0U)
   U_MANAGE_SYSRETURN_VALUE(long,               "%ld",  ret == -1L)
   U_MANAGE_SYSRETURN_VALUE(long long,          "%lld", ret == -1LL)
   U_MANAGE_SYSRETURN_VALUE(unsigned long,      "%lu",  ret == 0UL)
   U_MANAGE_SYSRETURN_VALUE(unsigned long long, "%llu", ret == 0ULL)
   U_MANAGE_SYSRETURN_VALUE(float,              "%f",   ret == 0.0)
   U_MANAGE_SYSRETURN_VALUE(double,             "%g",   ret == 0.0)
   U_MANAGE_SYSRETURN_VALUE(pvoid_t,            "%p",   ret ==  0 ||
                                                        ret == (void*)-1)
   U_MANAGE_SYSRETURN_VALUE(pchar_t,            "%S",   ret ==  0)
   U_MANAGE_SYSRETURN_VALUE(pcchar_t,           "%S",   ret ==  0)
   U_MANAGE_SYSRETURN_VALUE(puchar_t,           "%S",   ret ==  0)
   U_MANAGE_SYSRETURN_VALUE(pcuchar_t,          "%S",   ret ==  0)
   U_MANAGE_SYSRETURN_VALUE(sighandler_t,       "%p",   ret == (sighandler_t)SIG_ERR)

   /*
    * Type of file sizes and offsets (LFS)
#if SIZEOF_OFF_T != SIZEOF_LONG
   U_MANAGE_SYSRETURN_VALUE(off_t,              "%I",   ret == -1LL)
#elif SIZEOF_OFF_T == 4 
   U_MANAGE_SYSRETURN_VALUE(loff_t,             "%I",   ret == -1LL)
#endif

   U_MANAGE_SYSRETURN_VALUE(x11error_t,         "%p",   ret == (x11error_t)0)
   U_MANAGE_SYSRETURN_VALUE(x11IOerror_t,       "%p",   ret == (x11IOerror_t)0)
   */

   // Attivazione-disattivazione temporanea

   static void  resume() { u_trace_suspend(1); }
   static void suspend() { u_trace_suspend(0); }

private:
   uint32_t buffer_trace_len, buffer_syscall_len;
   char buffer_trace[1019], buffer_syscall[1019];
   bool flag_syscall_read_or_write;
public:
   bool active;
};

#endif
