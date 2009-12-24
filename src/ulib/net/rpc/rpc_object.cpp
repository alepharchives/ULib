// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    rpc_object.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/file_config.h>
#include <ulib/net/server/server.h>
#include <ulib/net/rpc/rpc_object.h>
#include <ulib/net/rpc/rpc_envelope.h>

UString*    URPCObject::str_response_type_0;
UString*    URPCObject::str_response_type_1;
UString*    URPCObject::str_response_type_2;
UString*    URPCObject::str_response_type_3;
UString*    URPCObject::str_response_type_4;
UString*    URPCObject::str_response_type_5;
UString*    URPCObject::str_fault_reason;
URPCObject* URPCObject::dispatcher;

void URPCObject::str_allocate()
{
   U_TRACE(0, "URPCObject::str_allocate()")

   U_INTERNAL_ASSERT_EQUALS(str_response_type_0,0)
   U_INTERNAL_ASSERT_EQUALS(str_response_type_1,0)
   U_INTERNAL_ASSERT_EQUALS(str_response_type_2,0)
   U_INTERNAL_ASSERT_EQUALS(str_response_type_3,0)
   U_INTERNAL_ASSERT_EQUALS(str_response_type_4,0)
   U_INTERNAL_ASSERT_EQUALS(str_response_type_5,0)
   U_INTERNAL_ASSERT_EQUALS(str_fault_reason,0)

   static ustringrep stringrep_storage[] = {
      { U_STRINGREP_FROM_CONSTANT("success_or_failure") },
      { U_STRINGREP_FROM_CONSTANT("stdin_success_or_failure") },
      { U_STRINGREP_FROM_CONSTANT("standard_output") },
      { U_STRINGREP_FROM_CONSTANT("stdin_standard_output") },
      { U_STRINGREP_FROM_CONSTANT("standard_output_binary") },
      { U_STRINGREP_FROM_CONSTANT("stdin_standard_output_binary") },
      { U_STRINGREP_FROM_CONSTANT("The requested method does not exist on this server") }
   };

   U_NEW_ULIB_OBJECT(str_response_type_0,    U_STRING_FROM_STRINGREP_STORAGE(0));
   U_NEW_ULIB_OBJECT(str_response_type_1,    U_STRING_FROM_STRINGREP_STORAGE(1));
   U_NEW_ULIB_OBJECT(str_response_type_2,    U_STRING_FROM_STRINGREP_STORAGE(2));
   U_NEW_ULIB_OBJECT(str_response_type_3,    U_STRING_FROM_STRINGREP_STORAGE(3));
   U_NEW_ULIB_OBJECT(str_response_type_4,    U_STRING_FROM_STRINGREP_STORAGE(4));
   U_NEW_ULIB_OBJECT(str_response_type_5,    U_STRING_FROM_STRINGREP_STORAGE(5));
   U_NEW_ULIB_OBJECT(str_fault_reason,       U_STRING_FROM_STRINGREP_STORAGE(6));

   if (UServer_Base::str_METHOD_NAME == 0) UServer_Base::str_allocate();
}

void URPCObject::readFileMethod(UFileConfig& file_method)
{
   U_TRACE(0, "URPCObject::readFileMethod(%p)", &file_method)

   int rtype;
   UCommand* command;
   UString method_name, response_type;

   while (file_method.load())
      {
      method_name   = file_method[*UServer_Base::str_METHOD_NAME];
      response_type = file_method[*UServer_Base::str_RESPONSE_TYPE];

      rtype  = (response_type == *str_response_type_0 ?       success_or_failure     :
                response_type == *str_response_type_1 ? stdin_success_or_failure     :
                response_type == *str_response_type_2 ?       standard_output        :
                response_type == *str_response_type_3 ? stdin_standard_output        :
                response_type == *str_response_type_4 ?       standard_output_binary :
                                                        stdin_standard_output_binary);

      command = UServer_Base::loadConfigCommand(file_method, true);

      // Adds an object method to the list of method the object can call. Take ownership of the memory

      insertGenericMethod(method_name, command, rtype);

      file_method.clear();
      }

   U_ASSERT(methodList.empty() == false)
}

URPCMethod* URPCObject::find(const UString& methodName)
{
   U_TRACE(0, "URPCObject::find(%.*S)", U_STRING_TO_TRACE(methodName))

   URPCMethod* method;
   uint32_t n = methodList.size();

   // Iterate over the list of methods of the object

   for (uint32_t i = 0; i < n; ++i)
      {
      method = methodList[i];

      if (methodName == method->getMethodName()) U_RETURN_POINTER(method, URPCMethod);
      }

   U_RETURN_POINTER(0, URPCMethod);
}

UString URPCObject::processMessage(URPCEnvelope& theCall, bool& bContainsFault)
{
   U_TRACE(0, "URPCObject::processMessage(%p,%p)", &theCall, &bContainsFault)

   U_INTERNAL_ASSERT_POINTER(URPCMethod::encoder)

   UString retval;

   // Iterate over the list of methods

   URPCMethod* method = find(theCall.getMethodName());

   if (method == 0)
      {
      // Return object not found error. This would be a Client fault

      setFailed();

      URPCMethod::pFault->getFaultReason() = *str_fault_reason;

      bContainsFault = true;
      retval         = URPCMethod::encoder->encodeFault(URPCMethod::pFault);
      }
   else
      {
      bContainsFault = (method->execute(theCall) == false);
      retval         = URPCMethod::encoder->encodeMethodResponse(*method, theCall.getNsName());
      }

   U_RETURN_STRING(retval);
}

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* URPCObject::dump(bool reset) const
{
   *UObjectIO::os << "dispatcher (URPCObject           " << (void*)dispatcher  << ")\n"
                  << "methodList (UVector<URPCMethod*> " << (void*)&methodList << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
