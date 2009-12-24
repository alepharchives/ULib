/** ============================================================================
//
// = LIBRARY
//    ulibase - c library
//
// = FILENAME
//    gzio.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================ */

#ifndef ULIBASE_GZIO_H
#define ULIBASE_GZIO_H 1

#include <ulib/base/base.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 
Synopsis: Compress and Decompresses the source buffer into the destination buffer
*/

/* #define GZIP_MAGIC "\037\213" Magic header for gzip files, 1F 8B */ 

extern U_EXPORT uint32_t u_gz_deflate(const char* input, uint32_t len, char* result);
extern U_EXPORT uint32_t u_gz_inflate(const char* input, uint32_t len, char* result);

#ifdef __cplusplus
}
#endif

#endif
