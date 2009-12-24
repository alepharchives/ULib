// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    signature.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/ssl/signature.h>

UString USignature::sign()
{
   U_TRACE(0, "USignature::sign()")

   U_INTERNAL_ASSERT_EQUALS(state,SIGN)

   unsigned size = EVP_PKEY_size(privateKey);

   UString signature(size, '\0');

   if (EVP_SignFinal(&context, (unsigned char*)signature.data(), &size, privateKey))
      {
      signature.size_adjust(size);
      }
   else
      {
      // sign failed

      signature.clear();
      }

   U_RETURN_STRING(signature);
}

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* USignature::dump(bool reset) const
{
   *UObjectIO::os << "state      " << state           << '\n'
                  << "context    " << (void*)&context << '\n'
                  << "algorithm  " << algorithm       << '\n'
                  << "publicKey  " << publicKey       << '\n'
                  << "privateKey " << privateKey;

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
