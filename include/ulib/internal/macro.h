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

#define U_SINGLE_READ     -1
#define U_TIMEOUT_MS      (30L * 1000L) /* 30 second connection/read timeout */
#define U_SSL_TIMEOUT_MS  (10L * 1000L) /* 10 second handshake       timeout */

// NB: for avoid mis-aligned we use 4 bytes...
#define U_LZOP_COMPRESS "\x89\x4c\x5a\x4f" // "\x89\x4c\x5a\x4f\x00\x0d\x0a\x1a\x0a"

// ----------------------------------------------------------------------------------------------------------------------
// NB: the value needs to be a stack type boundary, see UStringRep::checkIfMReserve()...
// ----------------------------------------------------------------------------------------------------------------------
#define U_CAPACITY (U_MAX_SIZE_PREALLOCATE - (1 + sizeof(ustringrep))) // UStringRep::max_size(U_MAX_SIZE_PREALLOCATE)
// ----------------------------------------------------------------------------------------------------------------------

#define U_STRING_LIMIT (((U_NOT_FOUND-sizeof(ustringrep))/sizeof(char))-4096)

/* Optimization if it is enough a resolution of one second */

#undef U_gettimeofday

#if defined(HAVE_PTHREAD_H) && defined(ENABLE_THREAD)
#  define U_gettimeofday { if (u_pthread_time == 0) (void) U_SYSCALL(gettimeofday, "%p,%p", u_now, 0); }
#else
#  define U_gettimeofday                            (void) U_SYSCALL(gettimeofday, "%p,%p", u_now, 0);
#endif

#endif
