// ============================================================================
//
// = LIBRARY
//    ulibdbg - c++ library
//
// = FILENAME
//    error_memory.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIBDBG_ERROR_MEMORY_H
#define ULIBDBG_ERROR_MEMORY_H 1

#include <ulib/internal/common.h>

// CLASS FOR MEMORY CORRUPTION TEST

class U_EXPORT UMemoryError {
public:
   const void* _this;

   // CONSTRUCTOR

    UMemoryError() {                                                                    _this = (void*)U_CHECK_MEMORY_SENTINEL; }
   ~UMemoryError() { U_ASSERT_MACRO(invariant(), "ERROR ON MEMORY", getErrorType(this)) _this = 0; }

   // ASSIGNMENT

   UMemoryError& operator=(const UMemoryError& o)
      {
      U_ASSERT_MACRO(o.invariant(), "ERROR ON MEMORY", o.getErrorType(&o))
      U_ASSERT_MACRO(  invariant(), "ERROR ON MEMORY",   getErrorType(this))

      return *this;
      }

   // TEST FOR MEMORY CORRUPTION

   bool invariant() const { return (_this == (void*)U_CHECK_MEMORY_SENTINEL); }

   const char* getErrorType(const void* pobj) const;
};

#endif
