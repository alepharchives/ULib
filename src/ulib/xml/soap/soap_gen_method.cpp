// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//   soap_gen_method.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/xml/soap/soap_gen_method.h>

const UString* USOAPGenericMethod::str_response;

void USOAPGenericMethod::str_allocate()
{
   U_TRACE(0, "USOAPGenericMethod::str_allocate()")

   U_INTERNAL_ASSERT_EQUALS(str_response,0)

   static ustringrep stringrep_storage[] = {
       { U_STRINGREP_FROM_CONSTANT("response") }
   };

   U_NEW_ULIB_OBJECT(str_response, U_STRING_FROM_STRINGREP_STORAGE(0));
}
