// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    server_rdb.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/net/server/server_rdb.h>

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* URDBServer::dump(bool reset) const
{
   UServer<UTCPSocket>::dump(false);

   *UObjectIO::os << '\n'
                  << "rdb           (URDB       " << (void*)rdb << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
