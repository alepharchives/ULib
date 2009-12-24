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

const char* UBison::dump(bool reset) const
{
   UFlexer::dump(false);

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
