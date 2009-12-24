// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    soap_gen_method.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_SOAP_GENERIC_METHOD_H
#define ULIB_SOAP_GENERIC_METHOD_H 1

#include <ulib/net/rpc/rpc_object.h>
#include <ulib/xml/soap/soap_fault.h>
#include <ulib/xml/soap/soap_encoder.h>
#include <ulib/net/rpc/rpc_gen_method.h>

class U_EXPORT USOAPGenericMethod : public URPCGenericMethod {
public:

   static UString* str_response;

   static void str_allocate();

   // COSTRUTTORI

   USOAPGenericMethod(const UString& n, UCommand* cmd, int rtype) : URPCGenericMethod(n, cmd, rtype)
      {
      U_TRACE_REGISTER_OBJECT(0, USOAPGenericMethod, "%.*S,%p,%d", U_STRING_TO_TRACE(n), cmd, rtype) 

      if (str_response == 0) str_allocate();
      }

   virtual ~USOAPGenericMethod()
      {
      U_TRACE_UNREGISTER_OBJECT(0, USOAPGenericMethod)
      }

   // VIRTUAL METHOD

   virtual void encode()
      {
      U_TRACE(0, "USOAPGenericMethod::encode()")

      U_INTERNAL_ASSERT_POINTER(URPCMethod::encoder)

      if      (hasFailed())                               U_SOAP_ENCODE_RES(                    response);
      else if (URPCObject::isOutputBinary(response_type)) U_SOAP_ENCB64_NAME_ARG(*str_response, response);
      else                                                U_SOAP_ENCODE_NAME_ARG(*str_response, response);
      }

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const { return USOAPGenericMethod::dump(reset); }
#endif

protected:

   // Triggers the creation of the USOAPFault

   virtual void setFailed()
      {
      U_TRACE(0, "USOAPGenericMethod::setFailed()")

      pFault = (URPCFault*) U_NEW(USOAPFault);
      }

private:
   USOAPGenericMethod(const USOAPGenericMethod& g) : URPCGenericMethod(UString::getStringNull(), 0, 0) {}
   USOAPGenericMethod& operator=(const USOAPGenericMethod& g)                                          { return *this; }
};

#endif
