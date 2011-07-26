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
#include <ulib/net/server/client_image.h>

// -----------------------------------------------------------------------------------------------------------------------
// Very simple RPC-like layer
//
// Requests and responses are build of little packets each containing a U_TOKEN_NM(4) byte ascii token,
// an 8-byte hex value or length, and optionally data corresponding to the length.
// -----------------------------------------------------------------------------------------------------------------------

UVector<UString>* URPC::rpc_info;

// Read a token and value

uint32_t URPC::readTokenInt(USocket* s, const char* token, UString& buffer, uint32_t& rstart)
{
   U_TRACE(0, "URPC::readTokenInt(%p,%S,%.*S,%u)", s, token, U_STRING_TO_TRACE(buffer), rstart)

   uint32_t value = 0;

   if (((buffer.size() >= (rstart + U_TOKEN_LN)) || USocketExt::read(s, buffer, U_TOKEN_LN)) &&
       (token == 0 || memcmp(buffer.c_pointer(rstart), token, U_TOKEN_NM) == 0))
      {
      value = u_hex2int(buffer.c_pointer(rstart + U_TOKEN_NM), 8);

      rstart += U_TOKEN_LN;

      U_INTERNAL_DUMP("rstart = %u", rstart)
      }

   U_RETURN(value);
}

// Read a token, and then the string data

uint32_t URPC::readTokenString(USocket* s, const char* token, UString& buffer, uint32_t& rstart, UString& data)
{
   U_TRACE(0, "URPC::readTokenString(%p,%S,%.*S,%u,%p)", s, token, U_STRING_TO_TRACE(buffer), rstart, &data)

   uint32_t value = readTokenInt(s, token, buffer, rstart);

   if (value &&
       ((buffer.size() >= (rstart + value)) || USocketExt::read(s, buffer, value)))
      {
      data = buffer.substr(rstart, value);

      U_INTERNAL_ASSERT_EQUALS(data.size(),value)

      rstart += data.size();

      U_INTERNAL_DUMP("rstart = %u", rstart)
      }

   U_RETURN(value);
}

// Read an vector of string from the network (Ex: "FIND00000001ARGV00000003foo")

uint32_t URPC::readTokenVector(USocket* s, const char* token, UString& buffer, UVector<UString>& vec)
{
   U_TRACE(0, "URPC::readTokenVector(%p,%S,%p,%p)", s, token, &buffer, &vec)

   uint32_t i      = 0,
            rstart = 0,
            argc   = readTokenInt(s, token, buffer, rstart);

   if (argc)
      {
      UString data;

      while (true)
         {
         if (readTokenString(s, "ARGV", buffer, rstart, data) == 0) break;

         vec.push(data);

         if (++i == argc) break;
         }
      }

   U_RETURN(rstart);
}

bool URPC::readRPCRequest(USocket* s, bool reset)
{
   U_TRACE(0, "URPC::readRPCRequest(%p,%b)", s, reset)

   if (reset) resetRPCInfo();

   if (UClientImage_Base::isPipeline() == false) UClientImage_Base::initAfterGenericRead();

   UClientImage_Base::size_request = readTokenVector(s, 0, *UClientImage_Base::rbuffer, *rpc_info);

   if (UClientImage_Base::size_request)
      {
      UClientImage_Base::manageRequestSize(false);

      U_RETURN(true);
      }

   U_RETURN(false);
}
