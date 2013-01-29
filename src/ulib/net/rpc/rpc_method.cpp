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

URPCFault*   URPCMethod::pFault;
URPCEncoder* URPCMethod::encoder;

const UString* URPCMethod::str_ns;
const UString* URPCMethod::str_done;
const UString* URPCMethod::str_fault;

void URPCMethod::str_allocate()
{
   U_TRACE(0, "URPCMethod::str_allocate()")

   U_INTERNAL_ASSERT_EQUALS(str_ns,0)
   U_INTERNAL_ASSERT_EQUALS(str_done,0)
   U_INTERNAL_ASSERT_EQUALS(str_fault,0)

   static ustringrep stringrep_storage[] = {
       { U_STRINGREP_FROM_CONSTANT("ns") },
       { U_STRINGREP_FROM_CONSTANT("DONE") },
       { U_STRINGREP_FROM_CONSTANT("ERR ") }
   };

   U_NEW_ULIB_OBJECT(str_ns,    U_STRING_FROM_STRINGREP_STORAGE(0));
   U_NEW_ULIB_OBJECT(str_done,  U_STRING_FROM_STRINGREP_STORAGE(1));
   U_NEW_ULIB_OBJECT(str_fault, U_STRING_FROM_STRINGREP_STORAGE(2));
}

URPCMethod::URPCMethod()
{
   U_TRACE_REGISTER_OBJECT(0, URPCMethod, "")

   if (str_done == 0) str_allocate();
}

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* URPCMethod::dump(bool reset) const
{
   U_CHECK_MEMORY

   *UObjectIO::os << "pFault      (URPCFault   " << (void*)pFault        << ")\n"
                  << "encoder     (URPCEncoder " << (void*)encoder       << ")\n"
                  << "ns          (UString     " << (void*)&ns           << ")\n"
                  << "method_name (UString     " << (void*)&method_name  << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
