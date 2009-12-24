// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    rpc_method.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/net/rpc/rpc_method.h>

UString*     URPCMethod::str_done;
UString*     URPCMethod::str_fault;
URPCFault*   URPCMethod::pFault;
URPCEncoder* URPCMethod::encoder;

void URPCMethod::str_allocate()
{
   U_TRACE(0, "URPCMethod::str_allocate()")

   U_INTERNAL_ASSERT_EQUALS(str_done,0)
   U_INTERNAL_ASSERT_EQUALS(str_fault,0)

   static ustringrep stringrep_storage[] = {
       { U_STRINGREP_FROM_CONSTANT("DONE") },
       { U_STRINGREP_FROM_CONSTANT("ERR ") }
   };

   U_NEW_ULIB_OBJECT(str_done,  U_STRING_FROM_STRINGREP_STORAGE(0));
   U_NEW_ULIB_OBJECT(str_fault, U_STRING_FROM_STRINGREP_STORAGE(1));
}

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* URPCMethod::dump(bool reset) const
{
   *UObjectIO::os << "pFault      (URPCFault   " << (void*)pFault  << ")\n"
                  << "encoder     (URPCEncoder " << (void*)encoder << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
