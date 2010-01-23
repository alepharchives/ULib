// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    rpc.cpp - RPC utility
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/net/rpc/rpc.h>

// -----------------------------------------------------------------------------------------------------------------------
// Very simple RPC-like layer
//
// Requests and responses are build of little packets each containing a U_TOKEN_NM-byte ascii token,
// an 8-byte hex value or length, and optionally data corresponding to the length.
// -----------------------------------------------------------------------------------------------------------------------

UVector<UString>* URPC::rpc_info;

// Read a token and value

uint32_t URPC::readTokenInt(USocket* s, const char* token, UString& buffer)
{
   U_TRACE(0, "URPC::readTokenInt(%p,%S,%p)", s, token, &buffer)

   uint32_t value = 0,
            start = buffer.size();

   if (USocketExt::read(s, buffer, U_TOKEN_LN) &&
       (token == 0 || memcmp(buffer.c_pointer(start), token, U_TOKEN_NM) == 0))
      {
      value = u_hex2int(buffer.c_pointer(start + U_TOKEN_NM), 8);
      }

   U_RETURN(value);
}

// Read a token, and then the string data

uint32_t URPC::readTokenString(USocket* s, const char* token, UString& buffer, UString& data)
{
   U_TRACE(0, "URPC::readTokenString(%p,%S,%p,%p)", s, token, &buffer, &data)

   uint32_t start = buffer.size(),
            value = readTokenInt(s, token, buffer);

   if (value &&
       USocketExt::read(s, buffer, value))
      {
      if (data.same(buffer) == false)
         {
         data = buffer.substr(start + U_TOKEN_LN);

         U_INTERNAL_ASSERT_EQUALS(data.size(),value)
         }
      }

   U_RETURN(value);
}

// Read an vector of string from the network

bool URPC::readTokenVector(USocket* s, const char* token, UString& buffer, UVector<UString>& vec)
{
   U_TRACE(0, "URPC::readTokenVector(%p,%S,%p,%p)", s, token, &buffer, &vec)

   U_ASSERT(buffer.empty() == true)

   uint32_t i    = 0,
            argc = readTokenInt(s, token, buffer);

   if (argc)
      {
      UString data;

      for (; i < argc; ++i)
         {
         if (readTokenString(s, "ARGV", buffer, data) == false) break;

         vec.push(data);
         }

      U_RETURN(i == argc);
      }

   // check if method without argument...

   U_INTERNAL_DUMP("buffer.size() = %u", buffer.size())

   bool result = (buffer.size() == U_TOKEN_LN);

   U_RETURN(result);
}
