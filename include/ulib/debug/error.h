// ============================================================================
//
// = LIBRARY
//    ulibdbg - c++ library
//
// = FILENAME
//    error.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIBDBG_ERROR_H
#define ULIBDBG_ERROR_H 1

#include <ulib/base/base.h>

struct U_EXPORT UError {

   static void stackDump();
};

#endif
