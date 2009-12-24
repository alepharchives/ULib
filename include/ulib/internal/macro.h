// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    macro.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_MACRO_H
#define ULIB_MACRO_H 1

#define U_MILLISEC    1000L
#define U_TIMEOUT     (20L * U_MILLISEC)
#define U_SINGLE_READ -1

#define U_SIZEOF_UStringRep sizeof(ustringrep)

/* NB: for avoid mis-aligned we use 4 bytes... */
#define U_LZOP_COMPRESS "\x89\x4c\x5a\x4f" /* "\x89\x4c\x5a\x4f\x00\x0d\x0a\x1a\x0a" */

#define U_CAPACITY (U_MAX_SIZE_PREALLOCATE - (1 + U_SIZEOF_UStringRep))

#endif
