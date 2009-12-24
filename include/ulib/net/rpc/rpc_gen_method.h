// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    rpc_gen_method.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_RPC_GENERIC_METHOD_H
#define ULIB_RPC_GENERIC_METHOD_H 1

#include <ulib/command.h>
#include <ulib/net/rpc/rpc_method.h>

class U_EXPORT URPCGenericMethod : public URPCMethod {
public:

   static UString* str_command_fault;
   static UString* str_command_not_started;

   static void str_allocate();

   // COSTRUTTORI

   URPCGenericMethod(const UString& n, UCommand* cmd, int rtype) : response_type(rtype), method_name(n), response(U_CAPACITY)
      {
      U_TRACE_REGISTER_OBJECT(0, URPCGenericMethod, "%.*S,%p,%d", U_STRING_TO_TRACE(n), cmd, rtype) 

      command = cmd;

      if (str_command_fault == 0) str_allocate();
      }

   virtual ~URPCGenericMethod()
      {
      U_TRACE_UNREGISTER_OBJECT(0, URPCGenericMethod)

      delete command;
      }

   // VIRTUAL METHOD

   // Used to get the name of the method. This is matched up using the URPCObject dispatcher to respond to a call

   virtual UString getMethodName() const { return method_name; }

   // Transforms the method into something that servers and clients can send. The encoder holds the actual
   // data while the client hands data to be entered in. This makes a whole lot more sense in the samples that
   // should have shipped with the library

   virtual void encode()
      {
      U_TRACE(0, "URPCGenericMethod::encode()")

      U_INTERNAL_ASSERT_POINTER(URPCMethod::encoder)

      U_RPC_ENCODE_RES((hasFailed() ? str_fault : str_done)->data(), response);
      }

   // Only to be called on the server by the object dispatcher. This method executes the call and returns true
   // if the call succeeded, false if it failed. URPCGenericMethod should keep any return data in a member
   // variable. The information will be returned via a call to encode

   virtual bool execute(URPCEnvelope& theCall);

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   UCommand* command;
   int response_type;
   UString method_name, response;

private:
   URPCGenericMethod(const URPCGenericMethod& g) : URPCMethod() {}
   URPCGenericMethod& operator=(const URPCGenericMethod& g)     { return *this; }
};

#endif
