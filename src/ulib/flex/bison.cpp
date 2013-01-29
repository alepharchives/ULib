// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    bison.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/flex/bison.h>

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UBison::dump(bool _reset) const
{
   U_CHECK_MEMORY

   UFlexer::dump(false);

   if (_reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
