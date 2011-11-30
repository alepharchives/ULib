/** ============================================================================
//
// = LIBRARY
//    ulibase - c library
//
// = FILENAME
//    error.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================ */

#ifndef ULIBASE_ERROR_H
#define ULIBASE_ERROR_H 1

#include <ulib/base/base.h>

#ifdef __cplusplus
extern "C" {
#endif

U_EXPORT void u_printError(void);

U_EXPORT const char* u_getSysError(                  uint32_t* restrict len); /* map errno number to an error message string */
U_EXPORT const char* u_getSysSignal(int signo,       uint32_t* restrict len); /* map signal number to an message string */
U_EXPORT const char* u_getExitStatus(int exit_value, uint32_t* restrict len); /* map exit status codes to an message string */

U_EXPORT void u_execOnExit(void); /* launch program on exit - ex. script to run gdb on process running */

#ifdef __cplusplus
}
#endif

#endif
