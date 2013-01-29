// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    rpc_envelope.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/net/rpc/rpc_envelope.h>

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* URPCEnvelope::dump(bool reset) const
{
   U_CHECK_MEMORY

   *UObjectIO::os << "mustUnderstand                              " << (void*)mustUnderstand << '\n'
                  << "arg                (UVector                 " << (void*)arg            << ")\n"
                  << "nsName             (UString                 " << (void*)&nsName        << ")\n"
                  << "methodName         (UString                 " << (void*)&methodName    << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
