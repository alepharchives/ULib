// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    base64.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/utility/base64.h>

void UBase64::encode(const unsigned char* s, uint32_t n, UString& buffer)
{
   U_TRACE(0, "UBase64::encode(%.*S,%u,%.*S)", n, s, n, U_STRING_TO_TRACE(buffer))

#ifdef DEBUG
   uint32_t length = ((n + 2) / 3) * 4, num_lines = (u_base64_max_columns ? length / u_base64_max_columns + 1 : 0);

   U_INTERNAL_DUMP("buffer.capacity() = %u length = %u num_lines = %u", buffer.capacity(), length, num_lines)

   U_ASSERT(buffer.capacity() >= length + num_lines + 1)
#endif

   uint32_t pos = u_base64_encode(s, n, (unsigned char*)buffer.data());

   buffer.size_adjust(pos);
}

bool UBase64::decode(const char* s, uint32_t n, UString& buffer)
{
   U_TRACE(0, "UBase64::decode(%.*S,%u,%.*S)", n, s, n, U_STRING_TO_TRACE(buffer))

   U_ASSERT(buffer.capacity() >= n)

   uint32_t pos = u_base64_decode(s, n, (unsigned char*)buffer.data());

   buffer.size_adjust(pos);

   bool result = (u_base64_errors == 0 && pos > 0);

   U_RETURN(result);
}
