// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    des3.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/utility/des3.h>
#include <ulib/utility/base64.h>

UString UDES3::signData(const char* fmt, ...)
{
   U_TRACE(0, "UDES3::::signData(%S)", fmt)

   UString buffer1(U_CAPACITY),
           buffer2(U_CAPACITY),
           signed_data(U_CAPACITY);

   va_list argp;
   va_start(argp, fmt);

   buffer1.vsnprintf(fmt, argp);

   va_end(argp);

            encode(buffer1, buffer2);
   UBase64::encode(buffer2, signed_data);

   U_RETURN_STRING(signed_data);
}
