// ============================================================================
//
// = LIBRARY
//    ulibdbg - c++ library
//
// = FILENAME
//    common.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIBDBG_COMMON_H
#define ULIBDBG_COMMON_H 1

extern U_EXPORT void  u_debug_init(void);
extern U_EXPORT pid_t u_debug_fork(pid_t pid);
extern U_EXPORT pid_t u_debug_vfork(pid_t pid);
extern U_EXPORT void  u_debug_exit(int exit_value);
extern U_EXPORT void  u_debug_exec(const char* pathname, char* const argv[], char* const envp[]);

// u_debug_set_memlimit() uses setrlimit() to restrict dynamic memory allocation.
// The argument to set_memlimit() is the limit in megabytes (a floating-point number).

extern U_EXPORT void u_debug_set_memlimit(float size);

#endif
