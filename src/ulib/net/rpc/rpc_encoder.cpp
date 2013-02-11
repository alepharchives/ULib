// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    rpc_encoder.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/net/rpc/rpc_encoder.h>

UString URPCEncoder::encodeMethod(URPCMethod& method, const UString& nsName) // namespace qualified element information
{
   U_TRACE(0, "URPCEncoder::encodeMethod(%p,%.*S)", &method, U_STRING_TO_TRACE(nsName))

   encodedValue.setEmpty();

   method.encode(); // Encode the method by virtual method...

   if (method.hasFailed() == false &&
       bIsResponse        == false)
      {
      UStringExt::buildTokenVector(method.getMethodName().data(), arg, encodedValue);
      }

   U_RETURN_STRING(encodedValue);
}

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* URPCEncoder::dump(bool reset) const
{
   *UObjectIO::os << "bIsResponse           " << bIsResponse          << '\n'
                  << "arg          (UVector " << (void*)&arg          << ")\n"
                  << "buffer       (UString " << (void*)&buffer       << ")\n"
                  << "encodedValue (UString " << (void*)&encodedValue << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
