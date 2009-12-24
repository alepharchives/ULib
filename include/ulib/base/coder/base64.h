/** ============================================================================
//
// = LIBRARY
//    ulibase - c library
//
// = FILENAME
//    base64.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================ */

#ifndef ULIBASE_BASE64_H
#define ULIBASE_BASE64_H 1

#include <ulib/base/base.h>

#ifdef __cplusplus
extern "C" {
#endif

extern U_EXPORT int u_base64_errors;

/* Encode-Decode base64 into a buffer */

extern U_EXPORT uint32_t u_base64_encode(const unsigned char* s, uint32_t n, unsigned char* result, int max_columns);
extern U_EXPORT uint32_t u_base64_decode(const unsigned char* s, uint32_t n, unsigned char* result);

#ifdef __cplusplus
}
#endif

#endif
