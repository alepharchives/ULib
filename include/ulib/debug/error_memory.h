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

#include <ulib/base/base.h>

// CLASS OF TEST FOR MEMORY CORRUPTION

#define U_CHECK_MEMORY_CLASS(m) U_ASSERT_MACRO((m).invariant(), "ERROR ON MEMORY", (m).getErrorType())

struct U_EXPORT UMemoryError {

   const UMemoryError* _this;

   // CONSTRUCTOR

    UMemoryError() {                             _this = this; }
   ~UMemoryError() { U_CHECK_MEMORY_CLASS(*this) _this = 0;    }

   // ASSIGNMENT

   UMemoryError& operator=(const UMemoryError& o)
      {
      U_CHECK_MEMORY_CLASS(*this)
      U_CHECK_MEMORY_CLASS(o)

      return *this;
      }

   UMemoryError(const UMemoryError& o) : _this(this) { U_CHECK_MEMORY_CLASS(o) }

   // TEST FOR MEMORY CORRUPTION

   bool invariant() const { return (this == _this); }

   static char buffer[64];

   const char* getErrorType() const
      {
      // Array Beyond Write | Free Memory Read...

      if (this != _this) (void) sprintf(buffer, "[this = %p _this = %p - %s]", this, _this, (_this ? "ABW" : "FMR"));

      return buffer;
      }
};

#endif
