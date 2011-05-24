// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    soap_fault.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/xml/soap/soap_fault.h>

const UString* USOAPFault::str_env_fault;

void USOAPFault::str_allocate()
{
   U_TRACE(0, "USOAPFault::str_allocate()")

   U_INTERNAL_ASSERT_EQUALS(str_env_fault,0)

   static ustringrep stringrep_storage[] = {
      { U_STRINGREP_FROM_CONSTANT("<?xml version='1.0' ?>"
                                  "<soap:Envelope xmlns:soap=\"http://www.w3.org/2003/05/soap-envelope\">"
                                       "<soap:Body>"
                                          "<soap:Fault>"
                                             "<soap:Code>"
                                                "<soap:Value>soap:%.*s</soap:Value>"
                                             "</soap:Code>"
                                             "<soap:Reason>"
                                                "<soap:Text xml:lang=\"en-US\">%.*s</soap:Text>"
                                             "</soap:Reason>"
                                             "<soap:Detail>%.*s</soap:Detail>"
                                          "</soap:Fault>"
                                       "</soap:Body>"
                                  "</soap:Envelope>") }
   };

   U_NEW_ULIB_OBJECT(str_env_fault, U_STRING_FROM_STRINGREP_STORAGE(0));
}
