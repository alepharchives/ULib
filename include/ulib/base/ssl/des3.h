// ============================================================================
//
// = LIBRARY
//    ulibase - c++ library
//
// = FILENAME
//    des3.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIBASE_DES3_H
#define ULIBASE_DES3_H 1

#include <ulib/base/base.h>

#ifdef __cplusplus
extern "C" {
#endif

extern U_EXPORT void u_des_init(void);
extern U_EXPORT void u_des_reset(void);
extern U_EXPORT void u_des_key(const char* restrict str);
extern U_EXPORT long u_des_encode(const unsigned char* restrict inp, long len, unsigned char* restrict out);
extern U_EXPORT long u_des_decode(const unsigned char* restrict inp, long len, unsigned char* restrict out);

extern U_EXPORT void u_des3_init(void);
extern U_EXPORT void u_des3_reset(void);
extern U_EXPORT void u_des3_key(const char* restrict str);
extern U_EXPORT long u_des3_encode(const unsigned char* restrict inp, long len, unsigned char* restrict out);
extern U_EXPORT long u_des3_decode(const unsigned char* restrict inp, long len, unsigned char* restrict out);

#ifdef __cplusplus
}
#endif

#endif
