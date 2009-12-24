// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    rpc_fault.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_RPC_FAULT_H
#define ULIB_RPC_FAULT_H 1

#include <ulib/string.h>

/**
  @class URPCFault
*/

class U_EXPORT URPCFault {
public:

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   enum FaultCode { Sender, Receiver, DataEncodingUnknown, MustUnderstand, VersionMismatch };

   static UString* str_encode;
   static UString* str_sender;
   static UString* str_receiver;
   static UString* str_MustUnderstand;
   static UString* str_VersionMismatch;
   static UString* str_DataEncodingUnknown;

   static void str_allocate();

   // COSTRUTTORI

   URPCFault() : detail(U_CAPACITY)
      {
      U_TRACE_REGISTER_OBJECT(0, URPCFault, "", 0)

      faultCode = Sender;

      if (str_sender == 0) str_allocate();
      }

   virtual ~URPCFault()
      {
      U_TRACE_UNREGISTER_OBJECT(0, URPCFault)
      }

   // SERVICES

   // Gets the general reason why the call failed.
   // Returns a string combining the generic fault and any more specific fault data

   UString getFaultCode();

   // Sets the generic fault code

   void setFaultCode(FaultCode code = Receiver) { faultCode = code; }

   // Allows the caller to set the generic and specific reasons for the failure of a call

   void setDetail()
      {
      U_TRACE(0, "URPCFault::setDetail()")

      U_INTERNAL_ASSERT_MAJOR(u_buffer_len, 0)

      detail.snprintf("%.*s", u_buffer_len, u_buffer);

      u_buffer_len = 0;
      }

   void setDetail(const char* format, ...);

   UString& getDetail()      { return detail; }
   UString& getFaultReason() { return faultReason; }

   // Encodes the complete fault into a string

   virtual void encode(UString& response)
      {
      U_TRACE(0, "URPCFault::encode(%.*S)", U_STRING_TO_TRACE(response))

      UString code = getFaultCode();

      response.setBuffer(str_encode->size() +
                         code.size()        +
                         faultReason.size() +
                         detail.size());

      response.snprintf(str_encode->data(), U_STRING_TO_TRACE(code),
                                            U_STRING_TO_TRACE(faultReason),
                                            (detail.empty() ? "" : " - "),
                                            U_STRING_TO_TRACE(detail));
      }

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   UString detail,         // Details behind why the fault occurred
           faultReason;    // Description of the fault
   FaultCode faultCode;    // Generic reason the call failed

private:
   URPCFault(const URPCFault&)            {}
   URPCFault& operator=(const URPCFault&) { return *this; }
};

#endif
