/** ============================================================================
//
// = LIBRARY
//    ulibase - c library
//
// = FILENAME
//    hash.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================ */

#ifndef ULIBASE_HASH_H
#define ULIBASE_HASH_H 1

#include <ulib/base/base.h>

#define hashsize(n) (1U<<(n))
#define hashmask(n) (hashsize(n)-1)

/* get next prime number for container */

#define U_GET_NEXT_PRIME_NUMBER(sz) ((sz) <=         53U ?         97U : \
                                     (sz) <=         97U ?        193U : \
                                     (sz) <=        193U ?        389U : \
                                     (sz) <=        389U ?        769U : \
                                     (sz) <=        769U ?       1543U : \
                                     (sz) <=       1543U ?       3079U : \
                                     (sz) <=       3079U ?       6151U : \
                                     (sz) <=       6151U ?      12289U : \
                                     (sz) <=      12289U ?      24593U : \
                                     (sz) <=      24593U ?      49157U : \
                                     (sz) <=      49157U ?      98317U : \
                                     (sz) <=      98317U ?     196613U : \
                                     (sz) <=     196613U ?     393241U : \
                                     (sz) <=     393241U ?     786433U : \
                                     (sz) <=     786433U ?    1572869U : \
                                     (sz) <=    1572869U ?    3145739U : \
                                     (sz) <=    3145739U ?    6291469U : \
                                     (sz) <=    6291469U ?   12582917U : \
                                     (sz) <=   12582917U ?   25165843U : \
                                     (sz) <=   25165843U ?   50331653U : \
                                     (sz) <=   50331653U ?  100663319U : \
                                     (sz) <=  100663319U ?  201326611U : \
                                     (sz) <=  201326611U ?  402653189U : \
                                     (sz) <=  402653189U ?  805306457U : \
                                     (sz) <=  805306457U ?  805306457U : \
                                     (sz) <= 1610612741U ? 3221225473U : 4294967291U)

#ifdef __cplusplus
extern "C" {
#endif

extern U_EXPORT uint32_t u_random(uint32_t a);                                      /* quick 4byte hashing function */
#ifdef HAVE_ARCH64
extern U_EXPORT uint32_t u_random64(uint64_t ptr);
#endif
extern U_EXPORT uint32_t u_hash(  unsigned char* t, uint32_t tlen, bool ignore_case); /* hash variable-length key into 32-bit value */
/*
extern U_EXPORT uint64_t u_hash64(unsigned char* t, uint32_t tlen);
*/

#ifdef __cplusplus
}
#endif

#endif
