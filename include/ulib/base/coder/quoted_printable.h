/** ============================================================================
//
// = LIBRARY
//    ulibase - c library
//
// = FILENAME
//    quoted_printable.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================ */

#ifndef ULIBASE_QUOTED_PRINTABLE_H
#define ULIBASE_QUOTED_PRINTABLE_H 1

#include <ulib/base/base.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Encode-Decode quoted_printable into a buffer */

extern U_EXPORT uint32_t u_quoted_printable_encode(const unsigned char* s, uint32_t n, unsigned char* result);
extern U_EXPORT uint32_t u_quoted_printable_decode(const unsigned char* s, uint32_t n, unsigned char* result);

#ifdef __cplusplus
}
#endif

#endif
