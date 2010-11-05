// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    soap_fault.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_SOAP_FAULT_H
#define ULIB_SOAP_FAULT_H 1

#include <ulib/net/rpc/rpc_fault.h>

/**
  @class USOAPFault
*/

class U_EXPORT USOAPFault : public URPCFault {
public:

   static const UString* str_env_fault;

   static void str_allocate();

   // COSTRUTTORI

   USOAPFault()
      {
      U_TRACE_REGISTER_OBJECT(0, USOAPFault, "")

      if (str_env_fault == 0) str_allocate();
      }

   virtual ~USOAPFault()
      {
      U_TRACE_UNREGISTER_OBJECT(0, USOAPFault)
      }

   // Encodes the complete fault into a string

   virtual void encode(UString& response)
      {
      U_TRACE(0, "USOAPFault::encode(%.*S)", U_STRING_TO_TRACE(response))

      UString code = getFaultCode();

      response.setBuffer(str_env_fault->size() +
                         code.size()           +
                         faultReason.size()    +
                         detail.size());

      response.snprintf(str_env_fault->data(), U_STRING_TO_TRACE(code),
                                               U_STRING_TO_TRACE(faultReason),
                                               U_STRING_TO_TRACE(detail));
      }

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const { return URPCFault::dump(reset); }
#endif

private:
   USOAPFault(const USOAPFault&) : URPCFault() {}
   USOAPFault& operator=(const USOAPFault&)    { return *this; }
};

#endif
