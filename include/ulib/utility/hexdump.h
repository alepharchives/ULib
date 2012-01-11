// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    hexdump.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_HEXDUMP_H
#define ULIB_HEXDUMP_H 1

#include <ulib/base/coder/hexdump.h>

#include <ulib/string.h>

struct U_EXPORT UHexDump {

   static void encode(const unsigned char* s, uint32_t n, UString& buffer);
   static void encode(const UString& s,                   UString& buffer) { encode((const unsigned char*)U_STRING_TO_PARAM(s), buffer); }

   static void decode(const          char* s, uint32_t n, UString& buffer);
   static void decode(const UString& s,                   UString& buffer) { decode(                      U_STRING_TO_PARAM(s), buffer); }
};

#endif
